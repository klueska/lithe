/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#include <stdlib.h>
#include <errno.h>
#include <parlib/spinlock.h>
#include <parlib/atomic.h>
#include <parlib/arch.h>
#include <assert.h>

void spinlock_init(spinlock_t *lock)
{
  assert(lock);
  *lock = UNLOCKED;
}


int spinlock_trylock(spinlock_t *lock) 
{
  assert(lock);
  if (*lock == LOCKED)
    return LOCKED;

  return cmpxchg(lock, LOCKED, UNLOCKED);
}


void spinlock_lock(spinlock_t *lock) 
{
  assert(lock);
  while (spinlock_trylock(lock) != UNLOCKED)
    cpu_relax();
}


void spinlock_unlock(spinlock_t *lock) 
{
  assert(lock);
  *lock = UNLOCKED;
}
