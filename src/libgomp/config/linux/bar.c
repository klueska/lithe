/* Copyright (C) 2005, 2008 Free Software Foundation, Inc.
   Contributed by Richard Henderson <rth@redhat.com>.

   This file is part of the GNU OpenMP Library (libgomp).

   Libgomp is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   Libgomp is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
   more details.

   You should have received a copy of the GNU Lesser General Public License 
   along with libgomp; see the file COPYING.LIB.  If not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/* As a special exception, if you link this library with other files, some
   of which are compiled with GCC, to produce an executable, this library
   does not by itself cause the resulting executable to be covered by the
   GNU General Public License.  This exception does not however invalidate
   any other reasons why the executable file might be covered by the GNU
   General Public License.  */

/* This is a Linux specific implementation of a barrier synchronization
   mechanism for libgomp.  This type is private to the library.  This 
   implementation uses atomic instructions and the futex syscall.  */

#include <limits.h>
#include "wait.h"

#if __x86_64__ && USE_LITHE

#include "libgomp.h"

static void bar_block(lithe_task_t *task, void *arg)
{
  struct gomp_thread *thr = (struct gomp_thread *) task->tls;
  thr->lithe_task = task;

  if (thr->state == BLOCKING) {
    if (!__sync_bool_compare_and_swap(&thr->state, BLOCKING, BLOCKED)) {
      assert(thr->state == RUNNING);
      thr->lithe_task = NULL;
      lithe_task_resume(task);
    }
  } else {
    assert(thr->state == RUNNING);
    thr->lithe_task = NULL;
    lithe_task_resume(task);
  }

  lithe_sched_reenter();
}

void
gomp_barrier_wait_end (gomp_barrier_t *bar, gomp_barrier_state_t last)
{
  if (last) {
    bar->generation++;

    bar->awaited = bar->total;

    struct gomp_sched *sched = (struct gomp_sched *) lithe_sched_this();
    int i;
    for (i = 0; i < sched->gomp_threads_size; i++) {
      if (sched->gomp_threads[i].state == BLOCKING) {
	if (!__sync_bool_compare_and_swap(&(sched->gomp_threads[i].state), BLOCKING, RUNNING)) {
	  assert(sched->gomp_threads[i].state == BLOCKED);
	  sched->gomp_threads[i].state = RESUMABLE;
	}
      } else if (sched->gomp_threads[i].state == BLOCKED) {
	sched->gomp_threads[i].state = RESUMABLE;
      }
    }

/*     asm volatile ("" : : : "memory"); */
/*     sched->gomp_threads_team_barrier_generation = bar->generation; */
  } else {
    /* TODO(benh): This is such a hack ... */
    lithe_task_t *task;
    lithe_task_get(&task);
    void *tls = task->tls;
    assert(tls != NULL ? gomp_thread()->ts.team->prev_ts.team == ((struct gomp_thread *) tls)->ts.team : 1);
    task->tls = gomp_thread();
    lithe_task_block(bar_block, NULL);
    gomp_tls_data = task->tls;
    task->tls = tls;
  }
}

gomp_barrier_state_t gomp_barrier_wait_start (gomp_barrier_t *bar)
{
/*   struct gomp_sched *sched = (struct gomp_sched *) lithe_sched_this(); */
/*   sched->gomp_threads_at_team_barrier = true; */
  assert(gomp_thread()->state == RUNNING);
  gomp_thread()->state = BLOCKING;
  return __sync_add_and_fetch (&bar->awaited, -1) == 0;
}

void
gomp_barrier_wait (gomp_barrier_t *bar)
{
  assert (bar == &(gomp_thread()->ts.team->barrier));
  gomp_barrier_wait_end (bar, gomp_barrier_wait_start (bar));
}

void
gomp_team_barrier_wait (gomp_barrier_t *bar)
{
  assert (bar == &(gomp_thread()->ts.team->barrier));
  gomp_barrier_wait_end (bar, gomp_barrier_wait_start (bar));
}

void
gomp_team_barrier_wait_end (gomp_barrier_t *bar, gomp_barrier_state_t state)
{
  fprintf(stderr, "unimplemented (%s:%d)\n", __FILE__, __LINE__);
  abort();
}

void gomp_team_barrier_wake (gomp_barrier_t *bar, int count)
{
  fprintf(stderr, "unimplemented (%s:%d)\n", __FILE__, __LINE__);
  abort();
}

#else
void
gomp_barrier_wait_end (gomp_barrier_t *bar, gomp_barrier_state_t state)
{
  if (__builtin_expect ((state & 1) != 0, 0))
    {
      /* Next time we'll be awaiting TOTAL threads again.  */
      bar->awaited = bar->total;
      atomic_write_barrier ();
      bar->generation += 4;
      futex_wake ((int *) &bar->generation, INT_MAX);
    }
  else
    {
      unsigned int generation = state;

      do
	do_wait ((int *) &bar->generation, generation);
      while (bar->generation == generation);
    }
}

void
gomp_barrier_wait (gomp_barrier_t *bar)
{
  gomp_barrier_wait_end (bar, gomp_barrier_wait_start (bar));
}

/* Like gomp_barrier_wait, except that if the encountering thread
   is not the last one to hit the barrier, it returns immediately.
   The intended usage is that a thread which intends to gomp_barrier_destroy
   this barrier calls gomp_barrier_wait, while all other threads
   call gomp_barrier_wait_last.  When gomp_barrier_wait returns,
   the barrier can be safely destroyed.  */

void
gomp_barrier_wait_last (gomp_barrier_t *bar)
{
  gomp_barrier_state_t state = gomp_barrier_wait_start (bar);
  if (state & 1)
    gomp_barrier_wait_end (bar, state);
}

void
gomp_team_barrier_wake (gomp_barrier_t *bar, int count)
{
  futex_wake ((int *) &bar->generation, count == 0 ? INT_MAX : count);
}

void
gomp_team_barrier_wait_end (gomp_barrier_t *bar, gomp_barrier_state_t state)
{
  unsigned int generation;

  if (__builtin_expect ((state & 1) != 0, 0))
    {
      /* Next time we'll be awaiting TOTAL threads again.  */
      struct gomp_thread *thr = gomp_thread ();
      struct gomp_team *team = thr->ts.team;
      bar->awaited = bar->total;
      atomic_write_barrier ();
      if (__builtin_expect (team->task_count, 0))
	{
	  gomp_barrier_handle_tasks (state);
	  state &= ~1;
	}
      else
	{
	  bar->generation = state + 3;
	  futex_wake ((int *) &bar->generation, INT_MAX);
	  return;
	}
    }

  generation = state;
  do
    {
      do_wait ((int *) &bar->generation, generation);
      if (__builtin_expect (bar->generation & 1, 0))
	gomp_barrier_handle_tasks (state);
      if ((bar->generation & 2))
	generation |= 2;
    }
  while (bar->generation != state + 4);
}

void
gomp_team_barrier_wait (gomp_barrier_t *bar)
{
  gomp_team_barrier_wait_end (bar, gomp_barrier_wait_start (bar));
}
#endif /* __x86_64__ && USE_LITHE */
