/**
 * Implementation of non-reentrant, non-error catching, unfair
 * mutexes.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ucontext.h>

#include <lithe/lithe_mutex.h>
#include <ht/atomic.h>
#include <spinlock.h>


DEFINE_TYPED_DEQUE(task, lithe_task_t *);


int lithe_mutex_init(lithe_mutex_t *mutex)
{
  if (mutex == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* Do initialization. */
  spinlock_init(&mutex->lock);
  mutex->locked = false;
  task_deque_init(&mutex->deque);

  return 0;
}


void block(lithe_task_t *task, void *arg)
{
  lithe_mutex_t *mutex = (lithe_mutex_t *) arg;
  task_deque_enqueue(&mutex->deque, task);
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
      lithe_task_block(block, mutex);
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

  lithe_task_t *task = NULL;

  spinlock_lock(&mutex->lock);
  {
    if (task_deque_length(&mutex->deque) > 0) {
      task_deque_dequeue(&mutex->deque, &task);
    }
    mutex->locked = false;
  }
  spinlock_unlock(&mutex->lock);

  if (task != NULL) {
    lithe_task_unblock(task);
  }
  
  return 0;
}
