/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#include <stdlib.h>
#include <errno.h>
#include <parlib/spinlock.h>
#include <ht/atomic.h>
#include <ht/arch.h>

int spinlock_init(int *lock)
{
  if (lock == NULL) {
    errno = EINVAL;
    return -1;
  }

  *lock = UNLOCKED;

  return 0;
}


int spinlock_trylock(int *lock) 
{
  if (lock == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (*lock == LOCKED)
    return LOCKED;

  return cmpxchg(lock, LOCKED, UNLOCKED);
}


int spinlock_lock(int *lock) 
{
  if (lock == NULL) {
    errno = EINVAL;
    return -1;
  }

  while (spinlock_trylock(lock) != UNLOCKED)
    cpu_relax();

  return 0;
}


int spinlock_unlock(int *lock) 
{
  if (lock == NULL) {
    errno = EINVAL;
    return -1;
  }

  *lock = UNLOCKED;

  return 0;
}
