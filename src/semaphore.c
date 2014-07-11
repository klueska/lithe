/* Copyright (c) 2012 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Implementation of lithe semaphores.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include "futex.h"
#include "semaphore.h"

int lithe_sem_init(lithe_sem_t *sem, int count)
{
  if(sem == NULL)
    return EINVAL;
  if(count < 0)
    return EINVAL;

  sem->value = count;
  sem->nwaiters = 0;
  return 0;
}

static int atomic_decrement_if_positive(int *pvalue)
{
  while (true) {
    int value = *pvalue;
    if (value > 0 && !__sync_bool_compare_and_swap(pvalue, value, value-1))
      continue;
    return value;
  }
}

int lithe_sem_wait(lithe_sem_t *sem)
{
  if(sem == NULL)
    return EINVAL;

  if (atomic_decrement_if_positive(&sem->value) > 0)
    return 0;

  __sync_fetch_and_add(&sem->nwaiters, 1);

  do {
    if (futex(&sem->value, FUTEX_WAIT, 0, NULL, NULL, 0))
      return -1;
  } while (atomic_decrement_if_positive(&sem->value) <= 0);

  return 0;
}

int lithe_sem_post(lithe_sem_t *sem)
{
  if(sem == NULL)
    return EINVAL;

  __sync_fetch_and_add(&sem->value, 1);
  __sync_synchronize();

  if (sem->nwaiters > 0)
    return futex(&sem->value, FUTEX_WAKE, 1, NULL, NULL, 0);

  return 0;
}

