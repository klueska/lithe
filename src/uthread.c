/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#include <parlib.h>
#include <vcore.h>
#include <uthread.h>
#include <errno.h>
#include <ht/atomic.h>
#include <ht/arch.h>
#include <ht/tls.h>

#define printd(...)

/* Which operations we'll call for the 2LS.  Will change a bit with Lithe.  For
 * now, there are no defaults.  2LSs can override sched_ops. */
struct schedule_ops default_2ls_ops = {0};
struct schedule_ops *sched_ops __attribute__((weak)) = &default_2ls_ops;

__thread struct uthread *current_uthread = 0;

/* static helpers: */
static int __uthread_allocate_tls(struct uthread *uthread);
static void __uthread_free_tls(struct uthread *uthread);

/* Gets called once out of uthread_create().  Can also do this in a ctor. */
int uthread_init(void)
{
	/* Only do this initialization once, every time after, just return 0 */
	static bool first = TRUE;
	if (!first) 
		return 0;
	first = FALSE;

	/* Make sure we are NOT in vcore context */
	assert(!in_vcore_context());

	/* Bug if vcore init was called with no 2LS */
	assert(sched_ops->sched_init);
	/* Get thread 0's thread struct (2LS allocs it) */
	struct uthread *uthread = sched_ops->sched_init();
	/* Associate the main thread's tls with the uthread returned from
     * sched_init(). */
	uthread->tls_desc = current_tls_desc;
    /* Set the current thread */
    current_uthread = uthread;
    /* Change temporarily to vcore0s tls region so we can save the newly created
     * tcb into its current_uthread variable and then restore it. */
    assert(current_tls_desc);
    void *temp_tls_desc = current_tls_desc;
    set_tls_desc(ht_tls_descs[0], 0);
    assert(in_vcore_context());
    current_uthread = uthread;
    set_tls_desc(temp_tls_desc, 0);

	/* Make sure we came back out of vcore context properly */
    assert(!in_vcore_context());

	/* Request some cores ! */
	while (num_vcores() < 1) {
		vcore_request(1);
		/* TODO: consider blocking */
		cpu_relax();
	}
	return 0;
}

/* 2LSs shouldn't call uthread_vcore_entry directly */
void __attribute__((noreturn)) uthread_vcore_entry(void)
{
	assert(in_vcore_context());
	assert(sched_ops->sched_entry);
	sched_ops->sched_entry();
	/* 2LS sched_entry should never return */
	assert(0);
}

/* Creates a uthread.  Will pass udata to sched_ops's thread_create.  For now,
 * the vcore/default 2ls code handles start routines and args.  Mostly because
 * this is used when initing a ucontext, which is vcore specific for now. */
struct uthread *uthread_create(void (*func)(void), void *udata)
{
	/* Make sure we are initialized */
	assert(uthread_init() == 0);

    /* Make sure we have a thread_create op registered */
	assert(sched_ops->thread_create);
	/* Call the thread_create op to create a new 'scheduler specific' thread */
	struct uthread *new_thread = sched_ops->thread_create(func, udata);

	/* Allocate a new tls region and associate it with the newly created thread */
	assert(current_tls_desc);
    /* Grab a reference to the vcoreid for future use */
	uint32_t vcoreid = vcore_id();
	/* Get a TLS for the new thread */
	assert(!__uthread_allocate_tls(new_thread));
	/* Switch into the new guys TLS and let it know who it is */
    void *temp_tls_desc = current_tls_desc;
	set_tls_desc(new_thread->tls_desc, vcoreid);
	/* Save the new_thread to the current_thread in that uthread's TLS */
	current_uthread = new_thread;
	/* Switch back to the caller */
	set_tls_desc(temp_tls_desc, vcoreid);

	return new_thread;
}

void uthread_runnable(struct uthread *uthread)
{
	/* Allow the 2LS to make the thread runnable, and do whatever. */
	assert(sched_ops->thread_runnable);
	sched_ops->thread_runnable(uthread);
}

/* Need to have this as a separate, non-inlined function since we clobber the
 * stack pointer before calling it, and don't want the compiler to play games
 * with my hart. */
static void __attribute__((noinline, noreturn)) 
__uthread_yield(void)
{
	assert(in_vcore_context());
	assert(sched_ops->thread_yield);

	struct uthread *uthread = current_uthread;
	/* 2LS will save the thread somewhere for restarting.  Later on,
	 * we'll probably have a generic function for all sorts of waiting.
	 */
	sched_ops->thread_yield(uthread);

	/* Leave the current vcore completely */
	current_uthread = NULL;
	/* Go back to the entry point, where we can handle notifications or
	 * reschedule someone. */
	uthread_vcore_entry();
}

/* Calling thread yields.  TODO: combine similar code with uthread_exit() (done
 * like this to ease the transition to the 2LS-ops */
void uthread_yield(bool save_state)
{
	assert(!in_vcore_context());

	struct uthread *uthread = current_uthread;
	uint32_t vcoreid = vcore_id();
	printd("[U] Uthread %08p is yielding on vcore %d\n", uthread, vcoreid);

	volatile bool yielding = TRUE; /* signal to short circuit when restarting */
	wrfence();
	/* Take the current state and save it into uthread->uc when this pthread
	 * restarts, it will continue from right after this, see yielding is false,
	 * and short circuit the function. */
	if(save_state) {
		int ret = getcontext(&uthread->uc);
		assert(ret == 0);
	}
	if (!yielding)
		goto yield_return_path;
	yielding = FALSE; /* for when it starts back up */
	/* Change to the transition context (both TLS and stack). */
	set_tls_desc(ht_tls_descs[vcoreid], vcoreid);
	assert(current_uthread == uthread);	
	assert(in_vcore_context());	/* technically, we aren't fully in vcore context */
	/* After this, make sure you don't use local variables. */
	set_stack_pointer(ht_context.uc_stack.ss_sp + ht_context.uc_stack.ss_size);
	wrfence();
	/* Finish yielding in another function. */
	__uthread_yield();
	/* Should never get here */
	assert(0);
	/* Will jump here when the uthread's trapframe is restarted/popped. */
yield_return_path:
	printd("[U] Uthread %08p returning from a yield!\n", uthread);
}

/* Destroys the uthread.  If you want to destroy a currently running uthread,
 * you'll want something like pthread_exit(), which yields, and calls this from
 * its sched_ops yield. */
void uthread_destroy(struct uthread *uthread)
{
	printd("[U] thread %08p on vcore %d is DYING!\n", uthread, vcore_id());
	/* we alloc and manage the TLS, so lets get rid of it */
	__uthread_free_tls(uthread);
	assert(sched_ops->thread_destroy);
	sched_ops->thread_destroy(uthread);
}

/* Saves the state of the current uthread from the point at which it is called */
void save_current_uthread(struct uthread *uthread)
{
	int ret = getcontext(&uthread->uc);
	assert(ret == 0);
}

/* Runs whatever thread is vcore's current_uthread */
void run_current_uthread(void)
{
	assert(current_uthread);

	uint32_t vcoreid = vcore_id();
	struct ucontext *uc = &current_uthread->uc;
	set_tls_desc(current_uthread->tls_desc, vcoreid);
	setcontext(uc);
	assert(0);
}

/* Launches the uthread on the vcore.  Don't call this on current_uthread. */
void run_uthread(struct uthread *uthread)
{
	assert(uthread != current_uthread);
	assert(uthread->tls_desc);

	uint32_t vcoreid = vcore_id();
	vcore_set_tls_var(vcoreid, current_uthread, uthread);
	set_tls_desc(uthread->tls_desc, vcoreid);
	setcontext(&uthread->uc);
	assert(0);
}

/* Swaps the currently running uthread for a new one, saving the state of the
 * current uthread in the process */
void swap_uthreads(struct uthread *__old, struct uthread *__new)
{
  volatile bool swap = true;
  void *tls_desc = get_tls_desc(vcore_id());
  ucontext_t uc;
  getcontext(&uc);
  wrfence();
  if(swap) {
    swap = false;
    memcpy(&__old->uc, &uc, sizeof(ucontext_t));
    run_uthread(__new);
  }
  int vcoreid = vcore_id();
  vcore_set_tls_var(vcoreid, current_uthread, __old);
  set_tls_desc(tls_desc, vcoreid);
}

/* Deals with a pending preemption (checks, responds).  If the 2LS registered a
 * function, it will get run.  Returns true if you got preempted.  Called
 * 'check' instead of 'handle', since this isn't an event handler.  It's the "Oh
 * shit a preempt is on its way ASAP".  While it is isn't too involved with
 * uthreads, it is tied in to sched_ops. */
bool check_preempt_pending(uint32_t vcoreid)
{
	bool retval = FALSE;
//	if (__procinfo.vcoremap[vcoreid].preempt_pending) {
//		retval = TRUE;
//		if (sched_ops->preempt_pending)
//			sched_ops->preempt_pending();
//		/* this tries to yield, but will pop back up if this was a spurious
//		 * preempt_pending. */
//		sys_yield(TRUE);
//	}
	return retval;
}

/* TLS helpers */
static int __uthread_allocate_tls(struct uthread *uthread)
{
	uthread->tls_desc = allocate_tls();
	if (!uthread->tls_desc) {
		errno = ENOMEM;
		return -1;
	}
	return 0;
}

static void __uthread_free_tls(struct uthread *uthread)
{
	free_tls(uthread->tls_desc);
	uthread->tls_desc = NULL;
}

