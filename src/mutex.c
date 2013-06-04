/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Implementation of non-reentrant, non-error catching, unfair
 * mutexes.
 */

#include <errno.h>
#include "internal/assert.h"
#include <stdbool.h>
#include <stdlib.h>

#include <parlib/parlib.h>
#include <parlib/mcs.h>
#include "mutex.h"

int lithe_mutexattr_init(lithe_mutexattr_t *attr)
{
  if(attr == NULL)
    return EINVAL;
  attr->type = LITHE_MUTEX_DEFAULT;
}

int lithe_mutexattr_settype(lithe_mutexattr_t *attr, int type)
{
  if(attr == NULL)
    return EINVAL;
  if(type >= NUM_LITHE_MUTEX_TYPES)
	return EINVAL;
  attr->type = type;
  return 0;
}

int lithe_mutexattr_gettype(lithe_mutexattr_t *attr, int *type)
{
  if(attr == NULL)
    return EINVAL;
  *type = attr->type;
  return 0;
}

int lithe_mutex_init(lithe_mutex_t *mutex, lithe_mutexattr_t *attr)
{
  if(mutex == NULL)
    return EINVAL;
  if(attr == NULL)
    lithe_mutexattr_init(&mutex->attr);
  else
    mutex->attr = *attr;

  /* Do initialization. */
  TAILQ_INIT(&mutex->queue);
  mcs_lock_init(&mutex->lock);
  mutex->qnode = NULL;
  mutex->locked = 0;
  mutex->owner = NULL;
  return 0;
}

static void block(lithe_context_t *context, void *arg)
{
  lithe_mutex_t *mutex = (lithe_mutex_t *) arg;
  assert(mutex);
  TAILQ_INSERT_TAIL(&mutex->queue, context, link);
  mcs_lock_unlock(&mutex->lock, mutex->qnode);
}

int lithe_mutex_trylock(lithe_mutex_t *mutex)
{
  if(mutex == NULL)
    return EINVAL;

  int retval = 0;
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&mutex->lock, &qnode);
  if(mutex->attr.type == LITHE_MUTEX_RECURSIVE &&
     mutex->owner == lithe_context_self()) {
    mutex->locked++;
  }
  else if(mutex->locked) {
    retval = EBUSY;
  }
  else {
    mutex->owner = lithe_context_self();
    mutex->locked++;
  }
  mcs_lock_unlock(&mutex->lock, &qnode);
  return retval;
}

int lithe_mutex_lock(lithe_mutex_t *mutex)
{
  if(mutex == NULL)
    return EINVAL;

  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&mutex->lock, &qnode);
  if(mutex->attr.type == LITHE_MUTEX_RECURSIVE &&
     mutex->owner == lithe_context_self()) {
    mutex->locked++;
  }
  else {
    while(mutex->locked) {
      mutex->qnode = &qnode;
      lithe_context_block(block, mutex);

      memset(&qnode, 0, sizeof(mcs_lock_qnode_t));
      mcs_lock_lock(&mutex->lock, &qnode);
    }
    mutex->owner = lithe_context_self();
    mutex->locked++;
  }
  mcs_lock_unlock(&mutex->lock, &qnode);
  return 0;
}

int lithe_mutex_unlock(lithe_mutex_t *mutex)
{
  if(mutex == NULL)
    return EINVAL;

  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&mutex->lock, &qnode);
  mutex->locked--;
  if(mutex->locked == 0) {
    lithe_context_t *context = TAILQ_FIRST(&mutex->queue);
    if(context)
      TAILQ_REMOVE(&mutex->queue, context, link);
    mutex->owner = NULL;
    mcs_lock_unlock(&mutex->lock, &qnode);

    if(context != NULL) {
      lithe_context_unblock(context);
    }
  }
  else {
    mcs_lock_unlock(&mutex->lock, &qnode);
  }
  return 0;
}

