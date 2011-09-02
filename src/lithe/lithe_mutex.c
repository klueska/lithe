/**
 * Implementation of non-reentrant, non-error catching, unfair
 * mutexes.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <atomic.h>
#include <spinlock.h>
#include <lithe/lithe_mutex.h>


DEFINE_TYPED_DEQUE(context, lithe_context_t *);


int lithe_mutex_init(lithe_mutex_t *mutex)
{
  if (mutex == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* Do initialization. */
  spinlock_init(&mutex->lock);
  mutex->locked = false;
  context_deque_init(&mutex->deque);

  return 0;
}


void block(lithe_context_t *context, void *arg)
{
  lithe_mutex_t *mutex = (lithe_mutex_t *) arg;
  context_deque_enqueue(&mutex->deque, context);
  spinlock_unlock(&mutex->lock);
}


int lithe_mutex_lock(lithe_mutex_t *mutex)
{
  if (mutex == NULL) {
    errno = EINVAL;
    return -1;
  }

  spinlock_lock(&mutex->lock);
  {
    while (mutex->locked) {
      lithe_context_block(block, mutex);
      spinlock_lock(&mutex->lock);
    }
    
    mutex->locked = true;
  }
  spinlock_unlock(&mutex->lock);

  return 0;
}


int lithe_mutex_unlock(lithe_mutex_t *mutex)
{
  if (mutex == NULL) {
    errno = EINVAL;
    return -1;
  }

  lithe_context_t *context = NULL;

  spinlock_lock(&mutex->lock);
  {
    if (context_deque_length(&mutex->deque) > 0) {
      context_deque_dequeue(&mutex->deque, &context);
    }
    mutex->locked = false;
  }
  spinlock_unlock(&mutex->lock);

  if (context != NULL) {
    lithe_context_unblock(context);
  }
  
  return 0;
}
