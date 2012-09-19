/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Interface of non-reentrant, non-error catching, unfair mutexes.
 */

#ifndef LITHE_MUTEX_H
#define LITHE_MUTEX_H

#include <parlib/mcs.h>
#include "lithe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lithe_mutex {
  mcs_lock_t lock;
  mcs_lock_qnode_t *qnode;
  int locked;
  struct lithe_context_deque deque;
} lithe_mutex_t;

/* Initialize a mutex. */
int lithe_mutex_init(lithe_mutex_t *mutex);

/* Try and lock a mutex. */
int lithe_mutex_trylock(lithe_mutex_t *mutex);

/* Lock a mutex. */
int lithe_mutex_lock(lithe_mutex_t *mutex);

/* Unlock a mutex. */
int lithe_mutex_unlock(lithe_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif // LITHE_MUTEX_H
