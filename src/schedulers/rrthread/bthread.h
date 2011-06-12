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
/* The actual bthread_api.h file is #included at the bottom of this file Users
 * of the bthread API need to include the location of this file in their
 * include path in order to use bthreads.  Other bthread implementations should
 * follow the same pattern.
 */

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
	void *stack;
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

#define bthread_rwlock_t bthread_mutex_t
#define bthread_rwlockattr_t bthread_mutexattr_t

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
#include <bthread_api.h>

#ifdef __cplusplus
  }
#endif

#endif
