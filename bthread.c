/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#include <bthread.h>
#include <vcore.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <parlib.h>
#include <ht/atomic.h>
#include <ht/arch.h>
#include <ht/mcs.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <assert.h>

#define printd(...)

struct bthread_queue ready_queue = TAILQ_HEAD_INITIALIZER(ready_queue);
struct bthread_queue active_queue = TAILQ_HEAD_INITIALIZER(active_queue);
mcs_lock_t queue_lock = MCS_LOCK_INIT;
bthread_once_t init_once = BTHREAD_ONCE_INIT;
int threads_ready = 0;
int threads_active = 0;

/* Helper / local functions */
static int get_next_pid(void);
static inline void spin_to_sleep(unsigned int spins, unsigned int *spun);

/* Pthread 2LS operations */
struct uthread *pth_init(void);
void pth_sched_entry(void);
struct uthread *pth_thread_create(void (*func)(void), void *udata);
void pth_thread_runnable(struct uthread *uthread);
void pth_thread_yield(struct uthread *uthread);
void pth_thread_exit(struct uthread *uthread);
unsigned int pth_vcores_wanted(void);
void pth_preempt_pending(void);
void pth_spawn_thread(uintptr_t pc_start, void *data);

struct schedule_ops bthread_sched_ops = {
	pth_init,
	pth_sched_entry,
	pth_thread_create,
	pth_thread_runnable,
	pth_thread_yield,
	pth_thread_exit,
	pth_vcores_wanted,
	0, /* pth_preempt_pending, */
	0, /* pth_spawn_thread, */
};

/* Publish our sched_ops, overriding the weak defaults */
struct schedule_ops *sched_ops __attribute__((weak)) = &bthread_sched_ops;

/* Static helpers */
static void __bthread_free_stack(struct bthread_tcb *pt);
static int __bthread_allocate_stack(struct bthread_tcb *pt);

/* Do whatever init you want.  Return a uthread representing thread0 (int
 * main()) */
struct uthread *pth_init(void)
{
	struct mcs_lock_qnode local_qn = {0};
	/* Tell the kernel where and how we want to receive events.  This is just an
	 * example of what to do to have a notification turned on.  We're turning on
	 * USER_IPIs, posting events to vcore 0's vcpd, and telling the kernel to
	 * send to vcore 0.  Note sys_self_notify will ignore the vcoreid pref.
	 * Also note that enable_kevent() is just an example, and you probably want
	 * to use parts of event.c to do what you want. */
	//enable_kevent(EV_USER_IPI, 0, EVENT_IPI);

	/* Create a bthread_tcb for the main thread */
//	bthread_t t = mmap(0, sizeof(struct bthread_tcb),
//	                   PROT_READ|PROT_WRITE|PROT_EXEC,
//	                   MAP_SHARED|MAP_POPULATE|MAP_ANONYMOUS, -1, 0);
//	memset(t, 0, sizeof(struct bthread_tcb));
	bthread_t t = (bthread_t)calloc(1, sizeof(struct bthread_tcb));
	assert(t);
	t->id = get_next_pid();
	assert(t->id == 0);

	/* Put the new bthread on the active queue */
	mcs_lock_notifsafe(&queue_lock, &local_qn);
	threads_active++;
	TAILQ_INSERT_TAIL(&active_queue, t, next);
	mcs_unlock_notifsafe(&queue_lock, &local_qn);
	return (struct uthread*)t;
}

/* Called from vcore entry.  Options usually include restarting whoever was
 * running there before or running a new thread.  Events are handled out of
 * event.c (table of function pointers, stuff like that). */
void __attribute__((noreturn)) pth_sched_entry(void)
{
	if (current_uthread) {
		run_current_uthread();
		assert(0);
	}
	/* no one currently running, so lets get someone from the ready queue */
	struct bthread_tcb *new_thread = NULL;
	struct mcs_lock_qnode local_qn = {0};
	mcs_lock_notifsafe(&queue_lock, &local_qn);
	new_thread = TAILQ_FIRST(&ready_queue);
	if (new_thread) {
		TAILQ_REMOVE(&ready_queue, new_thread, next);
		TAILQ_INSERT_TAIL(&active_queue, new_thread, next);
		threads_active++;
		threads_ready--;
	}
	mcs_unlock_notifsafe(&queue_lock, &local_qn);
	/* Instead of yielding, you could spin, turn off the core, set an alarm,
	 * whatever.  You want some logic to decide this.  Uthread code wil have
	 * helpers for this (like how we provide run_uthread()) */
	if (!new_thread) {
		/* TODO: consider doing something more intelligent here */
		printd("[P] No threads, vcore %d is yielding\n", vcore_id());
		vcore_yield();
		assert(0);
	}
	run_uthread((struct uthread*)new_thread);
	assert(0);
}

/* Could move this, along with start_routine and arg, into the 2LSs */
static void __bthread_run(void)
{
	struct bthread_tcb *me = bthread_self();
	bthread_exit(me->start_routine(me->arg));
}

/* Responible for creating the uthread and initializing its user trap frame */
struct uthread *pth_thread_create(void (*func)(void), void *udata)
{
	struct bthread_tcb *bthread;
	bthread_attr_t *attr = (bthread_attr_t*)udata;
//	bthread = mmap(0, sizeof(struct bthread_tcb),
//	               PROT_READ|PROT_WRITE|PROT_EXEC,
//	               MAP_SHARED|MAP_POPULATE|MAP_ANONYMOUS, -1, 0);
//	memset(bthread, 0, sizeof(struct bthread_tcb));
	bthread = (bthread_t)calloc(1, sizeof(struct bthread_tcb));
	assert(bthread);
	bthread->stacksize = BTHREAD_STACK_SIZE;	/* default */
	bthread->id = get_next_pid();
	bthread->detached = FALSE;				/* default */
	bthread->finished = FALSE;				/* default */
	/* Respect the attributes */
	if (attr) {
		if (attr->stacksize)					/* don't set a 0 stacksize */
			bthread->stacksize = attr->stacksize;
		if (attr->detachstate == BTHREAD_CREATE_DETACHED)
			bthread->detached = TRUE;
	}
	/* allocate a stack */
	if (__bthread_allocate_stack(bthread))
		printf("We're fucked\n");
	/* Set the u_tf to start up in __bthread_run, which will call the real
	 * start_routine and pass it the arg.  Note those aren't set until later in
	 * bthread_create(). */
	init_user_context(&(bthread->uthread.uc), (uint32_t)__bthread_run, 
                      (uint32_t)(bthread->stacktop));
	return (struct uthread*)bthread;
}

void pth_thread_runnable(struct uthread *uthread)
{
	struct bthread_tcb *bthread = (struct bthread_tcb*)uthread;
	struct mcs_lock_qnode local_qn = {0};
	/* Insert the newly created thread into the ready queue of threads.
	 * It will be removed from this queue later when vcore_entry() comes up */
	mcs_lock_notifsafe(&queue_lock, &local_qn);
	TAILQ_INSERT_TAIL(&ready_queue, bthread, next);
	threads_ready++;
	mcs_unlock_notifsafe(&queue_lock, &local_qn);
}

/* The calling thread is yielding.  Do what you need to do to restart (like put
 * yourself on a runqueue), or do some accounting.  Eventually, this might be a
 * little more generic than just yield. */
void pth_thread_yield(struct uthread *uthread)
{
	struct bthread_tcb *bthread = (struct bthread_tcb*)uthread;
	struct mcs_lock_qnode local_qn = {0};
	/* Take from the active list, and put on the ready list (tail).  Don't do
	 * this until we are done completely with the thread, since it can be
	 * restarted somewhere else. */
	mcs_lock_notifsafe(&queue_lock, &local_qn);
	threads_active--;
	TAILQ_REMOVE(&active_queue, bthread, next);
	threads_ready++;
	TAILQ_INSERT_TAIL(&ready_queue, bthread, next);
	mcs_unlock_notifsafe(&queue_lock, &local_qn);
}

/* Thread is exiting, do your 2LS specific stuff.  You're in vcore context.
 * Don't use the thread's TLS or stack or anything. */
void pth_thread_exit(struct uthread *uthread)
{
	struct bthread_tcb *bthread = (struct bthread_tcb*)uthread;
	struct mcs_lock_qnode local_qn = {0};
	/* Remove from the active runqueue */
	mcs_lock_notifsafe(&queue_lock, &local_qn);
	threads_active--;
	TAILQ_REMOVE(&active_queue, bthread, next);
	/* Cleanup, mirroring pth_thread_create() */
	__bthread_free_stack(bthread);
	mcs_unlock_notifsafe(&queue_lock, &local_qn);

	/* TODO: race on detach state */
	if (bthread->detached) {
		free(bthread);
//		assert(!munmap(bthread, sizeof(struct bthread_tcb)));
	}
	else {
		/* Once we do this, our joiner can free us.  He won't free us if we're
		 * detached, but there is still a potential race there (since he's accessing
		 * someone who is freed.) */
		bthread->finished = TRUE;
	}
}

/* Returns how many *more* vcores we want.  Smarter schedulers should look at
 * the num_vcores() and how much work is going on to make this decision. */
unsigned int pth_vcores_wanted(void)
{
	return 1;
}

void pth_preempt_pending(void)
{
}

void pth_spawn_thread(uintptr_t pc_start, void *data)
{
}

/* Pthread interface stuff and helpers */

int bthread_attr_init(bthread_attr_t *a)
{
 	a->stacksize = BTHREAD_STACK_SIZE;
	a->detachstate = BTHREAD_CREATE_JOINABLE;
  	return 0;
}

int bthread_attr_destroy(bthread_attr_t *a)
{
	return 0;
}

static void __bthread_free_stack(struct bthread_tcb *pt)
{
//	free(pt->stacktop - pt->stacksize);
	assert(!munmap(pt->stacktop - pt->stacksize, pt->stacksize));
}

static int __bthread_allocate_stack(struct bthread_tcb *pt)
{
	assert(pt->stacksize);
//	void *stackbot = calloc(1, pt->stacksize);
	void* stackbot = mmap(0, pt->stacksize,
	                      PROT_READ|PROT_WRITE|PROT_EXEC,
	                      MAP_SHARED|MAP_POPULATE|MAP_ANONYMOUS, -1, 0);
	if (stackbot == MAP_FAILED)
		return -1; // errno set by mmap
	pt->stacktop = stackbot + pt->stacksize;
	return 0;
}

// Warning, this will reuse numbers eventually
static int get_next_pid(void)
{
	static uint32_t next_pid = 0;
	return next_pid++;
}

int bthread_attr_setstacksize(bthread_attr_t *attr, size_t stacksize)
{
	attr->stacksize = stacksize;
	return 0;
}
int bthread_attr_getstacksize(const bthread_attr_t *attr, size_t *stacksize)
{
	*stacksize = attr->stacksize;
	return 0;
}

int bthread_create(bthread_t* thread, const bthread_attr_t* attr,
                   void *(*start_routine)(void *), void* arg)
{
	struct bthread_tcb *bthread =
	       (struct bthread_tcb*)uthread_create(__bthread_run, (void*)attr);
	if (!bthread)
		return -1;
	bthread->start_routine = start_routine;
	bthread->arg = arg;
	uthread_runnable((struct uthread*)bthread);
	*thread = bthread;
	return 0;
}

int bthread_join(bthread_t thread, void** retval)
{
	/* Not sure if this is the right semantics.  There is a race if we deref
	 * thread and he is already freed (which would have happened if he was
	 * detached. */
	if (thread->detached) {
		printf("[bthread] trying to join on a detached bthread");
		return -1;
	}
	while (!thread->finished)
		bthread_yield();
	if (retval)
		*retval = thread->retval;
//	assert(!munmap(thread, sizeof(struct bthread_tcb)));
	free(thread);
	return 0;
}

int bthread_yield(void)
{
	uthread_yield();
	return 0;
}

int bthread_mutexattr_init(bthread_mutexattr_t* attr)
{
  attr->type = BTHREAD_MUTEX_DEFAULT;
  return 0;
}

int bthread_mutexattr_destroy(bthread_mutexattr_t* attr)
{
  return 0;
}

int bthread_attr_setdetachstate(bthread_attr_t *__attr, int __detachstate)
{
	__attr->detachstate = __detachstate;
	return 0;
}

int bthread_mutexattr_gettype(const bthread_mutexattr_t* attr, int* type)
{
  *type = attr ? attr->type : BTHREAD_MUTEX_DEFAULT;
  return 0;
}

int bthread_mutexattr_settype(bthread_mutexattr_t* attr, int type)
{
  if(type != BTHREAD_MUTEX_NORMAL)
    return EINVAL;
  attr->type = type;
  return 0;
}

int bthread_mutex_init(bthread_mutex_t* m, const bthread_mutexattr_t* attr)
{
  m->attr = attr;
  m->lock = 0;
  return 0;
}

/* Set *spun to 0 when calling this the first time.  It will yield after 'spins'
 * calls.  Use this for adaptive mutexes and such. */
static inline void spin_to_sleep(unsigned int spins, unsigned int *spun)
{
	if ((*spun)++ == spins) {
		bthread_yield();
		*spun = 0;
	}
}

int bthread_mutex_lock(bthread_mutex_t* m)
{
	unsigned int spinner = 0;
	while(bthread_mutex_trylock(m))
		while(*(volatile size_t*)&m->lock) {
			cpu_relax();
			spin_to_sleep(BTHREAD_MUTEX_SPINS, &spinner);
		}
	return 0;
}

int bthread_mutex_trylock(bthread_mutex_t* m)
{
  return atomic_exchange_acq(&m->lock,1) == 0 ? 0 : EBUSY;
}

int bthread_mutex_unlock(bthread_mutex_t* m)
{
  /* Need to prevent the compiler (and some arches) from reordering older
   * stores */
  wrfence();
  m->lock = 0;
  return 0;
}

int bthread_mutex_destroy(bthread_mutex_t* m)
{
  return 0;
}

int bthread_cond_init(bthread_cond_t *c, const bthread_condattr_t *a)
{
  c->attr = a;
  memset(c->waiters,0,sizeof(c->waiters));
  memset(c->in_use,0,sizeof(c->in_use));
  c->next_waiter = 0;
  return 0;
}

int bthread_cond_destroy(bthread_cond_t *c)
{
  return 0;
}

int bthread_cond_broadcast(bthread_cond_t *c)
{
  memset(c->waiters,0,sizeof(c->waiters));
  return 0;
}

int bthread_cond_signal(bthread_cond_t *c)
{
  int i;
  for(i = 0; i < MAX_BTHREADS; i++)
  {
    if(c->waiters[i])
    {
      c->waiters[i] = 0;
      break;
    }
  }
  return 0;
}

int bthread_cond_wait(bthread_cond_t *c, bthread_mutex_t *m)
{
  int old_waiter = c->next_waiter;
  int my_waiter = c->next_waiter;
  
  //allocate a slot
  while (atomic_exchange_acq(& (c->in_use[my_waiter]), SLOT_IN_USE) == SLOT_IN_USE)
  {
    my_waiter = (my_waiter + 1) % MAX_BTHREADS;
    assert (old_waiter != my_waiter);  // do not want to wrap around
  }
  c->waiters[my_waiter] = WAITER_WAITING;
  c->next_waiter = (my_waiter+1) % MAX_BTHREADS;  // race on next_waiter but ok, because it is advisary

  bthread_mutex_unlock(m);

  volatile int* poll = &c->waiters[my_waiter];
  while(*poll);
  c->in_use[my_waiter] = SLOT_FREE;
  bthread_mutex_lock(m);

  return 0;
}

int bthread_condattr_init(bthread_condattr_t *a)
{
  a = BTHREAD_PROCESS_PRIVATE;
  return 0;
}

int bthread_condattr_destroy(bthread_condattr_t *a)
{
  return 0;
}

int bthread_condattr_setpshared(bthread_condattr_t *a, int s)
{
  a->pshared = s;
  return 0;
}

int bthread_condattr_getpshared(bthread_condattr_t *a, int *s)
{
  *s = a->pshared;
  return 0;
}

bthread_t bthread_self()
{
  return (struct bthread_tcb*)current_uthread;
}

int bthread_equal(bthread_t t1, bthread_t t2)
{
  return t1 == t2;
}

/* This function cannot be migrated to a different vcore by the userspace
 * scheduler.  Will need to sort that shit out. */
void bthread_exit(void *ret)
{
	struct bthread_tcb *bthread = bthread_self();
	bthread->retval = ret;
	uthread_exit();
}

int bthread_once(bthread_once_t* once_control, void (*init_routine)(void))
{
  if(atomic_exchange_acq(once_control,1) == 0)
    init_routine();
  return 0;
}

int bthread_barrier_init(bthread_barrier_t* b, const bthread_barrierattr_t* a, int count)
{
  b->nprocs = b->count = count;
  b->sense = 0;
  bthread_mutex_init(&b->pmutex, 0);
  return 0;
}

int bthread_barrier_wait(bthread_barrier_t* b)
{
  unsigned int spinner = 0;
  int ls = !b->sense;

  bthread_mutex_lock(&b->pmutex);
  int count = --b->count;
  bthread_mutex_unlock(&b->pmutex);

  if(count == 0)
  {
    printd("Thread %d is last to hit the barrier, resetting...\n", bthread_self()->id);
    b->count = b->nprocs;
	wrfence();
    b->sense = ls;
    return BTHREAD_BARRIER_SERIAL_THREAD;
  }
  else
  {
    while(b->sense != ls) {
      cpu_relax();
      spin_to_sleep(BTHREAD_BARRIER_SPINS, &spinner);
    }
    return 0;
  }
}

int bthread_barrier_destroy(bthread_barrier_t* b)
{
  bthread_mutex_destroy(&b->pmutex);
  return 0;
}

int bthread_detach(bthread_t thread)
{
	thread->detached = TRUE;
	return 0;
}
