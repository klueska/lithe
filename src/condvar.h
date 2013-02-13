/* Copyright (c) 2012 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Interface of non-reentrant, non-error catching, unfair condition variables.
 */

#ifndef LITHE_CONDVAR_H
#define LITHE_CONDVAR_H

#include <parlib/mcs.h>
#include "lithe.h"
#include "mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lithe_condvar {
  mcs_lock_t lock;
  mcs_lock_qnode_t *waiting_qnode;
  lithe_mutex_t *waiting_mutex;
  struct lithe_context_queue queue;
} lithe_condvar_t;
#define LITHE_CONDVAR_INITIALIZER {{0}, NULL, NULL, {0}}

/* Initialize a condition variable. */
int lithe_condvar_init(lithe_condvar_t* c);

/* Wait on a condition variable */
int lithe_condvar_wait(lithe_condvar_t* c, lithe_mutex_t* m);

/* Signal the next lithe context waiting on the condition variable */
int lithe_condvar_signal(lithe_condvar_t* c);

/* Broadcast a signal to all lithe contexts waiting on the condition variable */
int lithe_condvar_broadcast(lithe_condvar_t* c);

#ifdef __cplusplus
}
#endif

#endif // LITHE_CONDVAR_H
