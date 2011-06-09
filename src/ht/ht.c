/*
 * Copyright (c) 2011 The Regents of the University of California
 * Benjamin Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

/**
 * Hard thread implementation.
 *
 * TODO(benh): Reevaluate stack design/implementation. Specifically,
 * it is unclear whether or not (or how) stacks should be allocated
 * (should a user be responsible, and if so when do they get
 * deallocated).
 */

#ifndef __GNUC__
# error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>

#include <ht/atomic.h>
#include <ht/htinternal.h>
#include <ht/tlsinternal.h>
#include <ht/futex.h>

/* Array of hard threads using clone to masquerade. */
struct hard_thread *__ht_threads = NULL;

/* Array of TLS descriptors to use for each hard thread. Separate from the hard
 * thread array so that we can access it transparently, as an array, outside of
 * the hard thread library. */
void **ht_tls_descs = NULL;

/* Id of the currently running hard thread. */
__thread int __ht_id = -1;

/* Per hard thread context with its own stack and TLS region */
__thread ucontext_t ht_context = { 0 };

/* Current context running on an ht, used when swapping contexts onto an ht */
__thread ucontext_t *current_ucontext = NULL;

/* MCS lock required to be held when yielding an ht.  This variable stores a
 * reference to that value as it is passed in via a call to ht_yield */
pthread_mutex_t ht_yield_lock = PTHREAD_MUTEX_INITIALIZER;

/* Current tls_desc for the running ht, used when swapping contexts onto an ht */
__thread void *current_tls_desc = NULL;

/* Delineates whether the current context running on the ht is the ht context
 * itself */
__thread bool __in_ht_context = false;

/* Mutex used to provide mutually exclusive access to our globals. */
static pthread_mutex_t __ht_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Number of currently allocated hard threads. */
static int __ht_num_hard_threads = 0;

/* Max number of hard threads this application has requested. */
static int __ht_max_hard_threads = 0;

/* Limit on the number of hard threads that can be allocated. */
static int __ht_limit_hard_threads = 0;

/* Global context associated with the main thread.  Used when swapping this
 * context over to ht0 */
static ucontext_t __main_context = { 0 };

/* Stack space to use when a hard thread yields without a stack. */
static __thread void *__ht_stack = NULL;

/* Flag used to indicate if the original main thread has completely migrated
 * over to a hard thread or not.  Required so as not to clobber the stack if
 * the "new" main thread is started on the hard threaad before the original one
 * is done with all of its stack operations */
static int original_main_done = false;

void __ht_entry_gate()
{
  assert(__in_ht_context);
  /* Cache a copy of htid so we don't lose track of it over system call
   * invocations in which TLS might change out from under us. */
  int htid = __ht_id;
  pthread_mutex_lock(&__ht_mutex);
  {
    /* Check and see if we should wait at the gate. */
    if (__ht_max_hard_threads > __ht_num_hard_threads) {
      printf("%d not waiting at gate\n", htid);
      pthread_mutex_unlock(&__ht_mutex);
      goto entry;
    }

    /* Update hard thread counts. */
    __ht_num_hard_threads--;
    __ht_max_hard_threads--;

    /* Deallocate the thread. */
    __ht_threads[htid].allocated = false;

    /* Signal that we are about to wait on the futex. */
    __ht_threads[htid].running = false;

    /* Unlock the application lock associated with a yield if there is one */ 
	if(__ht_threads[htid].yielding) {
      pthread_mutex_unlock(&ht_yield_lock);
    }
  }
  pthread_mutex_unlock(&__ht_mutex);

  /* Wait for this hard thread to get woken up. */
  futex_wait(&(__ht_threads[htid].running), false);

  /* Hard thread is awake. */
entry:
  /* Wait for the original main to reach the point of not requiring its stack
   * anymore.  TODO: Think of a better way to this.  Required for now because
   * we need to call pthread_mutex_unlock() in this thread, which causes a race
   * for using the main threads stack with the hard thread restarting it in
   * ht_entry(). */
  while(!original_main_done) wrfence();
  /* Use cached value of htid in case TLS changed out from under us while
   * waiting for this hard thread to get woken up. */
  assert(__ht_threads[htid].running == true);
  /* Restore the TLS associated with this hard thread's context */
  set_tls_desc(ht_tls_descs[htid], htid);
  /* Jump to the hard thread's entry point */
  ht_entry();

  fprintf(stderr, "ht: failed to invoke ht_yield\n");

  /* We never exit a hard thread ... we always park them and therefore
   * we never exit them. If we did we would need to take care not to
   * exit the main hard thread (experience shows that exits the
   * application) and we should probably detach any created hard threads
   * (but this may be debatable because of our implementation of
   * htls).
   */
  exit(1);
}

void *__ht_entry_trampoline(void *arg)
{
  assert(sizeof(void *) == sizeof(long int));

  int htid = (int) (long int) arg;

  /* Initialize the tls region to be used by this ht */
  init_tls(htid);
  set_tls_desc(ht_tls_descs[htid], htid);

  /* Assign the id to the tls variable */
  __ht_id = htid;

  /* Set it that we are in ht context */
  __in_ht_context = true;

  /* If this is the first ht, set current_ucontext to the main_context in
   * this guys TLS region. MUST be done here, so in proper TLS */
  static int once = true;
  if(once) {
    once = false;
    current_tls_desc = __ht_main_tls_desc;
    current_ucontext = &__main_context;
  }

  /*
   * We create stack space for the function 'setcontext' jumped to
   * after an invocation of ht_yield.
   */
  if ((__ht_stack = alloca(getpagesize())) == NULL) {
    fprintf(stderr, "ht: could not allocate space on the stack\n");
    exit(1);
  }

  __ht_stack = (void *)
    (((size_t) __ht_stack) + (getpagesize() - __alignof__(long double)));

  /*
   * We need to save a context because experience shows that we seg fault if we
   * clean up this hard thread (i.e. call exit) on a stack other than the
   * original stack (maybe because certain things are placed on the stack like
   * cleanup functions).
   */
  if (getcontext(&ht_context) < 0) {
    fprintf(stderr, "ht: could not get context\n");
    exit(1);
  }

  if ((ht_context.uc_stack.ss_sp = alloca(getpagesize())) == NULL) {
    fprintf(stderr, "ht: could not allocate space on the stack\n");
    exit(1);
  }

  ht_context.uc_stack.ss_size = getpagesize();
  ht_context.uc_link = 0;

  makecontext(&ht_context, (void (*) ()) __ht_entry_gate, 0);
  setcontext(&ht_context);

  /* We never exit a hard thread ... we always park them and therefore
   * we never exit them. If we did we would need to take care not to
   * exit the main thread (experience shows that exits the
   * application) and we should probably detach any created hard threads
   * (but this may be debatable because of our implementation of
   * htls).
   */
  fprintf(stderr, "ht: failed to invoke ht_yield\n");
  exit(1);
}

static void __create_hard_thread(int i)
{
  struct hard_thread *cht = &__ht_threads[i];
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  cpu_set_t c;
  CPU_ZERO(&c);
  CPU_SET(i, &c);
  if ((errno = pthread_attr_setaffinity_np(&attr,
                                    sizeof(cpu_set_t),
                                    &c)) != 0) {
    fprintf(stderr, "ht: could not set affinity of underlying pthread\n");
    exit(1);
  }
  if ((errno = pthread_attr_setstacksize(&attr, 4*PTHREAD_STACK_MIN)) != 0) {
    fprintf(stderr, "ht: could not set stack size of underlying pthread\n");
    exit(1);
  }
  if ((errno = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
    fprintf(stderr, "ht: could not set stack size of underlying pthread\n");
    exit(1);
  }
  
  /* Set the created flag for the thread we are about to spawn off */
  cht->created = true;

  /* Also up the thread counts and set the flags for allocated and running
   * until we get a chance to stop the thread and deallocate it in its entry
   * gate */
  cht->allocated = true;
  cht->running = true;
  __ht_num_hard_threads++;
  __ht_max_hard_threads++;

  if ((errno = pthread_create(&cht->thread,
                              &attr,
                              __ht_entry_trampoline,
                              (void *) (long int) i)) != 0) {
    fprintf(stderr, "ht: could not allocate underlying hard thread\n");
    exit(2);
  }
}

static int __ht_allocate(int k)
{
  int j = 0;
  for (; k > 0; k--) {
    for (int i = 0; i < __ht_limit_hard_threads; i++) {
      assert(__ht_threads[i].created);
      if (!__ht_threads[i].allocated) {
        assert(__ht_threads[i].running == false);
        __ht_threads[i].allocated = true;
        __ht_threads[i].running = true;
        futex_wakeup_one(&(__ht_threads[i].running));
        j++;
        break;
      }
    }
  }
  return j;
}

int ht_request(int k)
{
  return -1;
}

static int __ht_request_async(int k)
{
  /* Determine how many hard threads we can allocate. */
  int available = __ht_limit_hard_threads - __ht_max_hard_threads;
  k = available >= k ? k : available;

  /* Update hard thread counts. */
  __ht_max_hard_threads += k;

  /* Allocate as many as known available. */
  k = __ht_max_hard_threads - __ht_num_hard_threads;
  int j = __ht_allocate(k);
  if(k != j) {
    printf("&k, %p, k: %d\n", &k, k);
  }
  fflush(stdout);
  assert(k == j);

  /* Update hard thread counts. */
  __ht_num_hard_threads += j;

  return __ht_max_hard_threads;
}

int ht_request_async(int k)
{
  int hts = -1;
  pthread_mutex_lock(&__ht_mutex);
  {
    if (k < 0) {
      fprintf(stderr, "ht: decrementing requests is unimplemented\n");
      exit(1);
    } else {
      /* If this is the first ht requested, do something special */
      static int once = true;
      if(once) {
        getcontext(&__main_context);
        wrfence();
        if(once) {
          once = false;
          __ht_request_async(1);
          pthread_mutex_unlock(&__ht_mutex);
          /* Don't use the stack anymore! */
          original_main_done = true;
          /* Futex calls are forced inline */
          futex_wait(&original_main_done, true);
          assert(0);
        }
        pthread_mutex_lock(&__ht_mutex);
        hts = 1;
        k -=1;
      }
      /* Put in the rest of the request as normal */
      hts = __ht_request_async(k);
    }
  }
  pthread_mutex_unlock(&__ht_mutex);
  return hts;
}

void ht_yield()
{
  /* Make sure that the application has grabbed the yield lock before calling
   * yield */
  pthread_mutex_trylock(&ht_yield_lock);
  /* Let the rest of the code know we are in the process of yielding */
  __ht_threads[__ht_id].yielding = true;
  /* Jump to the transition stack allocated on this hard thread's underlying
   * stack. This is only used very quickly so we can run the setcontext code */
  set_stack_pointer(__ht_stack);
  /* Go back to the ht entry gate */
  setcontext(&ht_context);
}

int ht_id()
{
  return __ht_id;
}

int ht_num_hard_threads()
{
  pthread_mutex_lock(&__ht_mutex);
  int c = __ht_num_hard_threads;
  pthread_mutex_unlock(&__ht_mutex);
  return c;
}


int ht_max_hard_threads()
{
  pthread_mutex_lock(&__ht_mutex);
  int c = __ht_max_hard_threads;
  pthread_mutex_unlock(&__ht_mutex);
  return c;
}


int ht_limit_hard_threads()
{
  pthread_mutex_lock(&__ht_mutex);
  int c = __ht_limit_hard_threads;
  pthread_mutex_unlock(&__ht_mutex);
  return c;
}

static void __ht_init()
{
  char *limit = getenv("HT_LIMIT");
  if (limit != NULL) {
    __ht_limit_hard_threads = atoi(limit);
  } else {
    __ht_limit_hard_threads = get_nprocs();  
  }

  /* Never freed.  Just freed automatically when the program dies since hard
   * threads should be alive for the entire lifetime of the program. */
  __ht_threads = malloc(sizeof(struct hard_thread) * __ht_limit_hard_threads);
  ht_tls_descs = malloc(sizeof(uintptr_t) * __ht_limit_hard_threads);

  if (__ht_threads == NULL || ht_tls_descs == NULL) {
    fprintf(stderr, "ht: failed to initialize hard threads\n");
    exit(1);
  }

  /* Initialize. */
  for (int i = 0; i < __ht_limit_hard_threads; i++) {
    __ht_threads[i].created = true;
    __ht_threads[i].allocated = true;
    __ht_threads[i].running = true;
  }

  /* Create all the hard threads up front */
  for (int i = 0; i < __ht_limit_hard_threads; i++) {
    /* Create all the hard threads */
    __create_hard_thread(i);

	/* Make sure the threads have all started and are ready to be allocated
     * before moving on */
    while (__ht_threads[i].running == true) {
      atomic_delay();
      wrfence();
    }
  }
  ht_ready();
}

/* Default callback after __ht_init has finished */
static void __ht_ready()
{
	// Do nothing by default...
}
extern void ht_ready() __attribute__ ((weak, alias ("__ht_ready")));

/* Callback after tls constructor has finished */
void tls_ready()
{
  __ht_init();
}

/* Wrapper for locking hard threads */
int ht_lock(ht_mutex_t *mutex)
{
  return pthread_mutex_lock((pthread_mutex_t*)mutex);
}
/* Wrapper for trylocking hard threads */
int ht_trylock(ht_mutex_t *mutex)
{
  return pthread_mutex_trylock((pthread_mutex_t*)mutex);
}
/* Wrapper for unlocking hard threads */
int ht_unlock(ht_mutex_t *mutex) 
{
  return pthread_mutex_unlock((pthread_mutex_t*)mutex);
}

/* Clear pending, and try to handle events that came in between a previous call
 * to handle_events() and the clearing of pending.  While it's not a big deal,
 * we'll loop in case we catch any.  Will break out of this once there are no
 * events, and we will have send pending to 0. 
 *
 * Note that this won't catch every race/case of an incoming event.  Future
 * events will get caught in pop_ros_tf() */
void clear_notif_pending(uint32_t htid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

/* Only call this if you know what you are doing. */
static inline void __enable_notifs(uint32_t htid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

/* Enables notifs, and deals with missed notifs by self notifying.  This should
 * be rare, so the syscall overhead isn't a big deal. */
void enable_notifs(uint32_t htid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

void disable_notifs(uint32_t htid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

