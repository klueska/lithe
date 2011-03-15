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

/* #define USE_ASYNC_REQUEST_THREAD */
/* #define USE_FUTEX */
/* #define LAZILY_CREATE_THREADS */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <htinternal.h>
#include <tls.h>

#include <sys/sysinfo.h>
#include <sys/wait.h>

#ifdef USE_FUTEX
#include "futex.h"
#endif /* USE_FUTEX */


/* Applications hard thread entry. */
extern void entry(void);

/* Array of hard threads using pthreads to masquerade. */
struct hard_thread *__ht_threads = NULL;

/* Mutex used to provide mutually exclusive access to our globals. */
static pthread_mutex_t __ht_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Condition variable to be used for asynchronous hard thread requests. */
static pthread_cond_t __ht_condition = PTHREAD_COND_INITIALIZER;

/* Number of currently allocated hard threads. */
static int __ht_num_hard_threads = 0;

/* Max number of hard threads this application has requested. */
static int __ht_max_hard_threads = 0;

/* Limit on the number of hard threads that can be allocated. */
static int __ht_limit_hard_threads = 0;

/* Id of the currently running hard thread. */
__thread int __ht_id = -1;

/* Context we should return to before we invoke pthread_exit. */
__thread ucontext_t __ht_context = { 0 };

/* Stack space to use when a hard thread yields without a stack. */
__thread void *__ht_stack = NULL;


void __ht_entry_gate()
{
/*   printf("%d yielded\n", __ht_id); */
  pthread_mutex_lock(&__ht_mutex);
  {
    /* Check and see if we should wait at the gate. */
    if (__ht_max_hard_threads > __ht_num_hard_threads) {
      printf("%d not waiting at gate\n", __ht_id);
      pthread_mutex_unlock(&__ht_mutex);
      goto entry;
    }

    /* Deallocate the thread. */
    __ht_threads[__ht_id].allocated = false;

    /* Update hard thread counts. */
    __ht_num_hard_threads--;
    __ht_max_hard_threads--;

    /* Signal that we are about to wait on the futex. */
    __ht_threads[__ht_id].running = false;
  }
  pthread_mutex_unlock(&__ht_mutex);

#ifdef USE_FUTEX
  futex_wait(&(__ht_threads[__ht_id].running), false);
#else
  while (__ht_threads[__ht_id].running == false) {
    asm volatile ("rep; nop; mfence" : : : "memory");
  }
#endif /* USE_FUTEX */
  
 entry:
  assert(__ht_threads[__ht_id].running == true);
  entry();

  fprintf(stderr, "ht: failed to invoke ht_yield\n");

  /* We never exit a pthread ... we always park pthreads and therefore
   * we never exit them. If we did we would need to take care not to
   * exit the main pthread (experience shows that exits the
   * application) and we should probably detach any created pthreads
   * (but this may be debatable because of our implementation of
   * htls).
   */
  exit(1);
}


void * __ht_entry_trampoline(void *arg)
{
  assert(sizeof(void *) == sizeof(long int));

  int htid = (int) (long int) arg;
  /* Set up a new TLS region for this hard thread and switch to it */
  init_tls(htid);

  /* Assign the id to the tls variable */
  __ht_id = htid;

  /*
   * We create stack space for the function 'setcontext' jumped to
   * after an invocation of ht_yield.
   */
  if ((__ht_stack = alloca(getpagesize()*4)) == NULL) {
    fprintf(stderr, "ht: could not allocate space on the stack\n");
    exit(1);
  }

  __ht_stack = (void *)
    (((size_t) __ht_stack) + (getpagesize()*4 - __alignof__(long double)));

  /*
   * We need to save a context because experience shows that we seg
   * fault if we clean up this pthread (i.e. call pthread_exit) on a
   * stack other than the original stack (maybe because pthread has
   * placed certain things on the stack like cleanup functions).
   */
  if (getcontext(&__ht_context) < 0) {
    fprintf(stderr, "ht: could not get context\n");
    exit(1);
  }

  if ((__ht_context.uc_stack.ss_sp = alloca(getpagesize()*4)) == NULL) {
    fprintf(stderr, "ht: could not allocate space on the stack\n");
    exit(1);
  }

  __ht_context.uc_stack.ss_size = getpagesize()*4;
  __ht_context.uc_link = 0;

  makecontext(&__ht_context, (void (*) ()) __ht_entry_gate, 0);

#ifdef LAZILY_CREATE_THREADS
  entry();
#else
  setcontext(&__ht_context);
#endif /* LAZILY_CREATE_THREADS */

  fprintf(stderr, "ht: failed to invoke ht_yield\n");

  /* We never exit a pthread ... we always park pthreads and therefore
   * we never exit them. If we did we would need to take care not to
   * exit the main pthread (experience shows that exits the
   * application) and we should probably detach any created pthreads
   * (but this may be debatable because of our implementation of
   * htls).
   */
  exit(1);
}


int __ht_allocate(int k)
{
  int j = 0;
  for (; k > 0; k--) {
    for (int i = 0; i < __ht_limit_hard_threads; i++) {
#ifdef LAZILY_CREATE_THREADS
      if (!__ht_threads[i].created) {
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

	__ht_threads[i].created = true;
	__ht_threads[i].allocated = true;
	__ht_threads[i].running = true;

	if ((errno = pthread_create(&__ht_threads[i].thread,
				    &attr,
				    __ht_entry_trampoline,
				    (void *) (long int) i)) != 0) {
	  fprintf(stderr, "ht: could not allocate underlying pthread\n");
	  exit(1);
	}

	j++;
	break;
      } else if (!__ht_threads[i].allocated) {
#else
      assert(__ht_threads[i].created);
      if (!__ht_threads[i].allocated) {
#endif /* LAZILY_CREATE_THREADS */
	assert(__ht_threads[i].running == false);
	__ht_threads[i].allocated = true;
	__ht_threads[i].running = true;
#ifdef USE_FUTEX
	futex_wakeup_one(&(__ht_threads[i].running));
#else
	asm volatile ("" : : : "memory");
#endif /* USE_FUTEX */
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


int ht_request_async(int k)
{
  pthread_mutex_lock(&__ht_mutex);
  {
    if (k < 0) {
      fprintf(stderr, "ht: decrementing requests is unimplemented\n");
      exit(1);
      __ht_max_hard_threads = __ht_max_hard_threads + k;
      if (__ht_max_hard_threads < __ht_num_hard_threads) {
	__ht_max_hard_threads = __ht_num_hard_threads;
      }
    } else {
      /* Determine how many hard threads we can allocate. */
      int available = __ht_limit_hard_threads - __ht_max_hard_threads;
      k = available >= k ? k : available;

      /* Update hard thread counts. */
      __ht_max_hard_threads += k;

/*       printf("request async can allocate %d and max is %d\n", k, __ht_max_hard_threads); */

#ifdef USE_ASYNC_REQUEST_THREAD
      /* Signal async request thread. */
      if (pthread_cond_signal(&__ht_condition) != 0) {
	pthread_mutex_unlock(&__ht_mutex);
	return -1;
      }
#else
      /* Allocate as many as known available. */
      k = __ht_max_hard_threads - __ht_num_hard_threads;
      int j = __ht_allocate(k);

      assert(k == j);

      /* Update hard thread counts. */
      __ht_num_hard_threads += j;
#endif /*  USE_ASYNC_REQUEST_THREAD */
    }
  }
  pthread_mutex_unlock(&__ht_mutex);

  return __ht_max_hard_threads;
}


int ht_id()
{
  return __ht_id;
}


int ht_num_hard_threads()
{
  return __ht_num_hard_threads;
}


int ht_max_hard_threads()
{
  return __ht_max_hard_threads;
}


int ht_limit_hard_threads()
{
  return __ht_limit_hard_threads;
}


void * __ht_async_handler(void *arg)
{
  pthread_mutex_lock(&__ht_mutex);
  {
    /* Wait on condition variable for an asynchronous request. */
    do {
      pthread_cond_wait(&__ht_condition, &__ht_mutex);

      /* Allocate as many as known available. */
      int k = __ht_max_hard_threads - __ht_num_hard_threads;
      int j = __ht_allocate(k);

      assert(k == j);

      /* Update hard thread counts. */
      __ht_num_hard_threads += j;
    } while (true);
  }
  pthread_mutex_unlock(&__ht_mutex);
}


static void __attribute__((constructor)) __ht_init()
{
  char *limit = getenv("HT_LIMIT");
  if (limit != NULL) {
    __ht_limit_hard_threads = atoi(limit);
  } else {
    __ht_limit_hard_threads = get_nprocs();  
  }

  __ht_threads = malloc(sizeof(struct hard_thread) * __ht_limit_hard_threads);

  if (__ht_threads == NULL) {
    fprintf(stderr, "ht: failed to initialize hard threads\n");
    exit(1);
  }

  /* Initialize. */
  for (int i = 0; i < __ht_limit_hard_threads; i++) {
    __ht_threads[i].created = false;
    __ht_threads[i].allocated = false;
    __ht_threads[i].running = false;
  }

  /* Allocate asynchronous request thread. */
#ifdef USE_ASYNC_REQUEST_THREAD
  pthread_t thread;
  if (pthread_create(&thread, NULL, __ht_async_handler, NULL) != 0) {
    fprintf(stderr, "ht: could not allocate underlying pthread\n");
    exit(1);
  }
#endif /* USE_ASYNC_REQUEST_THREAD */

#ifndef LAZILY_CREATE_THREADS
  for (int i = 1; i < __ht_limit_hard_threads; i++) {
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

    __ht_threads[i].created = true;
    __ht_threads[i].allocated = true;
    __ht_threads[i].running = true;

    __ht_num_hard_threads++;
    __ht_max_hard_threads++;

    if ((errno = pthread_create(&__ht_threads[i].thread,
				&attr,
				__ht_entry_trampoline,
				(void *) (long int) i)) != 0) {
      fprintf(stderr, "ht: could not allocate underlying pthread\n");
      exit(1);
    }
  }
#endif /* LAZILY_CREATE_THREADS */

  /* Initialize the main thread. Assumes that this is the main thread ... */
  cpu_set_t c;
  CPU_ZERO(&c);
  CPU_SET(0, &c);

  if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &c) != 0) {
    fprintf(stderr, "ht: could not set affinity of underlying pthread\n");
    exit(1);
  }

  __ht_id = 0;

  /*
   * We create stack space for the function 'setcontext' jumped to
   * after an invocation of ht_yield.
   */
  if ((__ht_stack = malloc(getpagesize()*4)) == NULL) {
    fprintf(stderr, "ht: could not allocate memory\n");
    exit(1);
  }

  __ht_stack = (void *)
    (((size_t) __ht_stack) + (getpagesize()*4 - __alignof__(long double)));

  /* Initialize the context for launching into __ht_entry_gate. */
  if (getcontext(&__ht_context) < 0) {
    fprintf(stderr, "ht: could not get context\n");
    exit(1);
  }

  if ((__ht_context.uc_stack.ss_sp = malloc(getpagesize()*4)) == NULL) {
    fprintf(stderr, "ht: could not allocate memory\n");
    exit(1);
  }

  __ht_context.uc_stack.ss_size = getpagesize()*4;
  __ht_context.uc_link = 0;
    
  makecontext(&__ht_context, (void (*) ()) __ht_entry_gate, 0);

  __ht_threads[0].created = true;
  __ht_threads[0].allocated = true;
  __ht_threads[0].running = true;

  /* Update hard thread counts. */
  __ht_num_hard_threads += 1;
  __ht_max_hard_threads += 1;

  /* TODO(benh): Make sure async request thread is ready. */
}
