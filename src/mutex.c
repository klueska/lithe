/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Implementation of non-reentrant, non-error catching, unfair
 * mutexes.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <parlib/parlib.h>
#include <parlib/mcs.h>
#include "mutex.h"

int lithe_mutex_init(lithe_mutex_t *mutex)
{
  if (mutex == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* Do initialization. */
  mcs_lock_init(&mutex->lock);
  mutex->locked = false;
  lithe_context_deque_init(&mutex->deque);

  return 0;
}


void block(lithe_context_t *context, void *arg)
{
  lithe_mutex_t *mutex = (lithe_mutex_t *) arg;
  lithe_context_deque_enqueue(&mutex->deque, context);
  mcs_lock_unlock(&mutex->lock, mutex->qnode);
}

int lithe_mutex_trylock(lithe_mutex_t *mutex)
{
  if (mutex == NULL) {
    errno = EINVAL;
    return -1;
  }

  int retval = 0;
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&mutex->lock, &qnode);
  if (mutex->locked)
    retval = -1;
  else
    mutex->locked = true;
  mcs_lock_unlock(&mutex->lock, &qnode);
  return retval;
}

int lithe_mutex_lock(lithe_mutex_t *mutex)
{
  if (mutex == NULL) {
    errno = EINVAL;
    return -1;
  }

  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&mutex->lock, &qnode);
  {
    while (mutex->locked) {
      mutex->qnode = &qnode;
      lithe_context_block(block, mutex);

      memset(&qnode, 0, sizeof(mcs_lock_qnode_t));
      mcs_lock_lock(&mutex->lock, &qnode);
    }
    
    mutex->locked = true;
  }
  mcs_lock_unlock(&mutex->lock, &qnode);

  return 0;
}


int lithe_mutex_unlock(lithe_mutex_t *mutex)
{
  if (mutex == NULL) {
    errno = EINVAL;
    return -1;
  }

  lithe_context_t *context = NULL;

  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&mutex->lock, &qnode);
  {
    if (lithe_context_deque_length(&mutex->deque) > 0) {
      lithe_context_deque_dequeue(&mutex->deque, &context);
    }
    mutex->locked = false;
  }
  mcs_lock_unlock(&mutex->lock, &qnode);

  if (context != NULL) {
    lithe_context_unblock(context);
  }
  
  return 0;
}
