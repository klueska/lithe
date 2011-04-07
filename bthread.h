/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef _BTHREAD_H
#define _BTHREAD_H

#include <sys/queue.h>
#include <vcore.h>
#include <uthread.h>

#ifdef __cplusplus
  extern "C" {
#endif

/* Pthread struct.  First has to be the uthread struct, which the vcore code
 * will access directly (as if bthread_tcb is a struct uthread). */
struct bthread_tcb {
	struct uthread uthread;
	TAILQ_ENTRY(bthread_tcb) next;
	int finished;
	bool detached;
	uint32_t id;
	uint32_t stacksize;
	void *(*start_routine)(void*);
	void *arg;
	void *stacktop;
	void *retval;
};
typedef struct bthread_tcb* bthread_t;
TAILQ_HEAD(bthread_queue, bthread_tcb);

#define BTHREAD_ONCE_INIT 0
#define BTHREAD_BARRIER_SERIAL_THREAD 12345
#define BTHREAD_MUTEX_INITIALIZER {0}
#define BTHREAD_MUTEX_NORMAL 0
#define BTHREAD_MUTEX_DEFAULT BTHREAD_MUTEX_NORMAL
#define BTHREAD_MUTEX_SPINS 100 // totally arbitrary
#define BTHREAD_BARRIER_SPINS 100 // totally arbitrary
#define BTHREAD_COND_INITIALIZER {0}
#define BTHREAD_PROCESS_PRIVATE 0

typedef struct
{
  int type;
} bthread_mutexattr_t;

typedef struct
{
  const bthread_mutexattr_t* attr;
  int lock;
} bthread_mutex_t;

/* TODO: MAX_BTHREADS is arbitrarily defined for now.
 * It indicates the maximum number of threads that can wait on  
   the same cond var/ barrier concurrently. */

#define MAX_BTHREADS 32
typedef struct
{
  volatile int sense;
  int count;
  int nprocs;
  bthread_mutex_t pmutex;
} bthread_barrier_t;

#define WAITER_CLEARED 0
#define WAITER_WAITING 1
#define SLOT_FREE 0
#define SLOT_IN_USE 1

/* Detach state.  */
enum
{
  BTHREAD_CREATE_JOINABLE,
#define BTHREAD_CREATE_JOINABLE	BTHREAD_CREATE_JOINABLE
  BTHREAD_CREATE_DETACHED
#define BTHREAD_CREATE_DETACHED	BTHREAD_CREATE_DETACHED
};

// TODO: how big do we want these?  ideally, we want to be able to guard and map
// more space if we go too far.
#define BTHREAD_STACK_PAGES 4
#define BTHREAD_STACK_SIZE (BTHREAD_STACK_PAGES*PGSIZE)

typedef struct
{
  int pshared;
} bthread_condattr_t;


typedef struct
{
  const bthread_condattr_t* attr;
  int waiters[MAX_BTHREADS];
  int in_use[MAX_BTHREADS];
  int next_waiter; //start the search for an available waiter at this spot
} bthread_cond_t;
typedef struct 
{
	size_t stacksize;
	int detachstate;
} bthread_attr_t;
typedef int bthread_barrierattr_t;
typedef int bthread_once_t;
typedef void** bthread_key_t;

/* The bthreads API */
int bthread_attr_init(bthread_attr_t *);
int bthread_attr_destroy(bthread_attr_t *);
int bthread_create(bthread_t *, const bthread_attr_t *,
                   void *(*)(void *), void *);
int bthread_join(bthread_t, void **);
int bthread_yield(void);

int bthread_attr_setdetachstate(bthread_attr_t *__attr,int __detachstate);

int bthread_mutex_destroy(bthread_mutex_t *);
int bthread_mutex_init(bthread_mutex_t *, const bthread_mutexattr_t *);
int bthread_mutex_lock(bthread_mutex_t *);
int bthread_mutex_trylock(bthread_mutex_t *);
int bthread_mutex_unlock(bthread_mutex_t *);
int bthread_mutex_destroy(bthread_mutex_t *);

int bthread_mutexattr_init(bthread_mutexattr_t *);
int bthread_mutexattr_destroy(bthread_mutexattr_t *);
int bthread_mutexattr_gettype(const bthread_mutexattr_t *, int *);
int bthread_mutexattr_settype(bthread_mutexattr_t *, int);

int bthread_cond_init(bthread_cond_t *, const bthread_condattr_t *);
int bthread_cond_destroy(bthread_cond_t *);
int bthread_cond_broadcast(bthread_cond_t *);
int bthread_cond_signal(bthread_cond_t *);
int bthread_cond_wait(bthread_cond_t *, bthread_mutex_t *);

int bthread_condattr_init(bthread_condattr_t *);
int bthread_condattr_destroy(bthread_condattr_t *);
int bthread_condattr_setpshared(bthread_condattr_t *, int);
int bthread_condattr_getpshared(bthread_condattr_t *, int *);

#define bthread_rwlock_t bthread_mutex_t
#define bthread_rwlockattr_t bthread_mutexattr_t
#define bthread_rwlock_destroy bthread_mutex_destroy
#define bthread_rwlock_init bthread_mutex_init
#define bthread_rwlock_unlock bthread_mutex_unlock
#define bthread_rwlock_rdlock bthread_mutex_lock
#define bthread_rwlock_wrlock bthread_mutex_lock
#define bthread_rwlock_tryrdlock bthread_mutex_trylock
#define bthread_rwlock_trywrlock bthread_mutex_trylock

bthread_t bthread_self();
int bthread_equal(bthread_t t1, bthread_t t2);
void bthread_exit(void* ret);
int bthread_once(bthread_once_t* once_control, void (*init_routine)(void));

int bthread_barrier_init(bthread_barrier_t* b, const bthread_barrierattr_t* a, int count);
int bthread_barrier_wait(bthread_barrier_t* b);
int bthread_barrier_destroy(bthread_barrier_t* b);

//added for redis compile
int bthread_detach(bthread_t __th);
int bthread_attr_setstacksize(bthread_attr_t *attr, size_t stacksize);
int bthread_attr_getstacksize(const bthread_attr_t *attr, size_t *stacksize);

#ifdef __cplusplus
  }
#endif

#endif
