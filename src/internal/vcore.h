#ifndef LITHE_INTERNAL_VCORE_H
#define LITHE_INTERNAL_VCORE_H

#include <stdlib.h>
#include <sys/queue.h>
#include <parlib/vcore.h>
#include "assert.h"

static struct {
  int vcid;
} __attribute__((aligned(ARCH_CL_SIZE))) *wake_me_up;
static int max_spin_count = 300000;

static inline void lithe_vcore_init()
{
  wake_me_up = parlib_aligned_alloc(PGSIZE,
            sizeof(wake_me_up[0]) * max_vcores());
  assert(wake_me_up);
  memset(wake_me_up, 0, sizeof(wake_me_up[0]) * max_vcores());

  const char *spin_count_string = getenv("LITHE_SPIN_COUNT");
  if (spin_count_string != NULL)
    max_spin_count = atoi(spin_count_string);
}

static inline void maybe_vcore_yield()
{
  int *flag = &wake_me_up[vcore_id()].vcid;

  *flag = 1;
  // make a local copy of max_spin_count to avoid reloading it due to cpu_relax
  for (int spins = 0, max = max_spin_count; spins < max && *flag; spins++)
    cpu_relax();

  if (*flag && __sync_lock_test_and_set(flag, 0))
    vcore_yield(false);
}

static inline void maybe_vcore_request(int k)
{
  /* Try and wake up one of our spinning vcores. */
  for (int i = 0; i < max_vcores() && k > 0; i++)
    if (wake_me_up[i].vcid && __sync_lock_test_and_set(&wake_me_up[i].vcid, 0))
      k--;

  for (int i = 0; i < k; i++)
    if (vcore_request(1) < 0)
      break;
}

#endif
