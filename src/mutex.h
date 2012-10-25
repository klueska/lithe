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

/* Lithe mutex types */
enum {
  LITHE_MUTEX_NORMAL,
  LITHE_MUTEX_RECURSIVE,
  NUM_LITHE_MUTEX_TYPES,
};
#define LITHE_MUTEX_DEFAULT LITHE_MUTEX_NORMAL

/* A lithe mutexattr struct */
typedef struct lithe_mutexattr {
  int type;
} lithe_mutexattr_t;

/* Initialize a lithe mutexattr */
int lithe_mutexattr_init(lithe_mutexattr_t *attr);

/* Get and set the lithe mutexattr type */
int lithe_mutexattr_settype(lithe_mutexattr_t *attr, int type);
int lithe_mutexattr_gettype(lithe_mutexattr_t *attr, int *type);

/* A lithe mutex struct */
typedef struct lithe_mutex {
  mcs_lock_t lock;
  mcs_lock_qnode_t *qnode;
  int locked;
  struct lithe_context_deque deque;
  lithe_mutexattr_t attr;
  lithe_context_t *owner;
} lithe_mutex_t;

/* Initialize a mutex. */
int lithe_mutex_init(lithe_mutex_t *mutex, lithe_mutexattr_t *attr);

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
