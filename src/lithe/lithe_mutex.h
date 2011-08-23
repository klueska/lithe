/**
 * Interface of non-reentrant, non-error catching, unfair mutexes.
 */

#ifndef LITHE_MUTEX_H
#define LITHE_MUTEX_H

#include "lithe.h"
#include "deque.h"

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_TYPED_DEQUE(context, lithe_context_t *);

typedef struct lithe_mutex {
  int lock;
  int locked;
  struct context_deque deque;
} lithe_mutex_t;

/* Initialize a mutex. */
int lithe_mutex_init(lithe_mutex_t *mutex);

/* Lock a mutex. */
int lithe_mutex_lock(lithe_mutex_t *mutex);

/* Unlock a mutex. */
int lithe_mutex_unlock(lithe_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif // LITHE_MUTEX_H
