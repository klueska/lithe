/* Copyright (c) 2012 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Interface of lithe semaphores
 */

#ifndef LITHE_SEMAPHORE_H
#define LITHE_SEMAPHORE_H

#include <parlib/mcs.h>
#include "mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A lithe semaphore struct */
typedef struct lithe_sem {
  int count;
  mcs_lock_t lock;
  lithe_mutex_t mutex;
} lithe_sem_t;
#define LITHE_SEM_INITIALIZER {0, {0}, {0}}

/* Initialize a semaphore. */
int lithe_sem_init(lithe_sem_t *sem, int count);

/* Wait on a semaphore. */
int lithe_sem_wait(lithe_sem_t *sem);

/* Post on a semaphore. */
int lithe_sem_post(lithe_sem_t *sem);

#ifdef __cplusplus
}
#endif

#endif // LITHE_SEMAPHORE_H
