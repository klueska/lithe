#include <stdlib.h>
#include <errno.h>
#include <spinlock.h>
#include <ht/atomic.h>

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
    atomic_delay();

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
