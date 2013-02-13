/* Copyright (c) 2012 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Implementation of lithe semaphores.
 */

#include <errno.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "semaphore.h"

int lithe_sem_init(lithe_sem_t *sem, int count)
{
  if(sem == NULL)
    return EINVAL;
  if(count <= 0)
    return EINVAL;

  sem->count = count;
  mcs_lock_init(&sem->lock);
  lithe_mutex_init(&sem->mutex, NULL);
  return 0;
}

int lithe_sem_wait(lithe_sem_t *sem)
{
  if(sem == NULL)
    return EINVAL;

  mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
  mcs_lock_lock(&sem->lock, &qnode);
  if(sem->count <= 0) {
    sem->count--;
    mcs_lock_unlock(&sem->lock, &qnode);
    lithe_mutex_lock(&sem->mutex);
  }
  else {
    sem->count--;
    mcs_lock_unlock(&sem->lock, &qnode);
  }  
  return 0;
}

int lithe_sem_post(lithe_sem_t *sem)
{
  if(sem == NULL)
    return EINVAL;

  mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
  mcs_lock_lock(&sem->lock, &qnode);
  if(sem->count < 0)
    lithe_mutex_unlock(&sem->mutex);
  sem->count++;
  mcs_lock_unlock(&sem->lock, &qnode);
  return 0;
}

