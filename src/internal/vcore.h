#ifndef LITHE_INTERNAL_VCORE_H
#define LITHE_INTERNAL_VCORE_H

#include <sys/queue.h>

static const int LITHE_YIELD_SPIN_COUNT = 10000;

typedef struct yield_queue_entry {
  bool wake;
  TAILQ_ENTRY(yield_queue_entry) link;
} yield_queue_entry_t;
TAILQ_HEAD(yield_queue, yield_queue_entry);
static struct yield_queue yield_queue = TAILQ_HEAD_INITIALIZER(yield_queue);
static spinlock_t yield_queue_lock = SPINLOCK_INITIALIZER;

static inline void maybe_vcore_yield()
{
  yield_queue_entry_t e = {false, NULL};

  spinlock_lock(&yield_queue_lock);
  TAILQ_INSERT_HEAD(&yield_queue, &e, link);
  spinlock_unlock(&yield_queue_lock);

  for (int spins = 0; spins < LITHE_YIELD_SPIN_COUNT && !e.wake; spins++)
    cpu_relax();

  bool do_yield = true;
  spinlock_lock(&yield_queue_lock);
  if (e.wake)
    do_yield = false;
  TAILQ_REMOVE(&yield_queue, &e, link);
  spinlock_unlock(&yield_queue_lock);

  if (do_yield)
    vcore_yield(false);
}

static inline int maybe_vcore_request(int k)
{
  spinlock_lock(&yield_queue_lock);
  yield_queue_entry_t* e;
  TAILQ_FOREACH(e, &yield_queue, link)
  {
    if (k == 0)
      break;
    k--;
    e->wake = true;
  }
  spinlock_unlock(&yield_queue_lock);

  return vcore_request(k);
}

#endif
