/* Copyright (c) 2012 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Implementation of non-reentrant, non-error catching, unfair condition
 * variables.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <parlib/parlib.h>
#include "mutex.h"
#include "condvar.h"

/* Initialize a condition variable. */
int lithe_condvar_init(lithe_condvar_t* c) {
  if(c == NULL)
    return NULL;

  mcs_lock_init(&c->lock);
  lithe_context_deque_init(&c->queue);
  return 0;
}

static void block(lithe_context_t *context, void *arg)
{
  lithe_condvar_t *condvar = (lithe_condvar_t *) arg;
  lithe_context_deque_enqueue(&condvar->queue, context);
  lithe_mutex_unlock(condvar->waiting_mutex);
  mcs_lock_unlock(&condvar->lock, condvar->waiting_qnode);
}

/* Wait on a condition variable */
int lithe_condvar_wait(lithe_condvar_t* c, lithe_mutex_t* m) {
  if(c == NULL)
    return EINVAL;
  if(m == NULL)
    return EINVAL;

  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&c->lock, &qnode);
  c->waiting_mutex = m;
  c->waiting_qnode = &qnode;
  lithe_context_block(block, c);
  return lithe_mutex_lock(m);
}

/* Signal the next lithe context waiting on the condition variable */
int lithe_condvar_signal(lithe_condvar_t* c) {
  if(c == NULL)
    return EINVAL;

  lithe_context_t *context = NULL;
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&c->lock, &qnode);
  if (lithe_context_deque_length(&c->queue) > 0) {
    lithe_context_deque_dequeue(&c->queue, &context);
  }
  mcs_lock_unlock(&c->lock, &qnode);

  if (context != NULL) {
    lithe_context_unblock(context);
  }
  return 0;
}

/* Broadcast a signal to all lithe contexts waiting on the condition variable */
int lithe_condvar_broadcast(lithe_condvar_t* c) {
  if(c == NULL)
    return EINVAL;

  lithe_context_t *context = NULL;
  mcs_lock_qnode_t qnode = {0};
  while(1) {
    mcs_lock_lock(&c->lock, &qnode);
    if (lithe_context_deque_length(&c->queue) > 0) {
      lithe_context_deque_dequeue(&c->queue, &context);
    }
    else break;
    mcs_lock_unlock(&c->lock, &qnode);
    lithe_context_unblock(context);
    memset(&qnode, 0, sizeof(mcs_lock_qnode_t));
  }
  mcs_lock_unlock(&c->lock, &qnode);
  return 0;
}

