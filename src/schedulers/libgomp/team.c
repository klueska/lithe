/* Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
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

/* This file handles the maintainence of threads in response to team
   creation and termination.  */

#include "libgomp.h"
#include <stdlib.h>
#include <string.h>

#if __x86_64__ && USE_LITHE
#include <assert.h>
#include <stdio.h>

#include <ht/atomic.h>

#include <sys/sysinfo.h>
#endif /* __x86_64__ && USE_LITHE */

/* This attribute contains PTHREAD_CREATE_DETACHED.  */
pthread_attr_t gomp_thread_attr;

/* This key is for the thread destructor.  */
pthread_key_t gomp_thread_destructor;


#if __x86_64__ && USE_LITHE

DECLARE_DEFINE_TYPED_DEQUE(tasks, lithe_task_t *);

int tasks_deque_lock;
struct tasks_deque tasks_deque;

#ifdef HAVE_TLS
/* TODO(benh): Eliminate htls ... is it really that much of an optimization? */
htls struct gomp_thread *gomp_tls_data = NULL;
htls int started = 0;
htls int resumed = 0;
#else
#error "Lithe expects TLS."
#endif

#else

/* This is the libgomp per-thread data structure.  */
#ifdef HAVE_TLS
__thread struct gomp_thread gomp_tls_data;
#else
pthread_key_t gomp_tls_key;
#endif

/* This structure is used to communicate across pthread_create.  */

struct gomp_thread_start_data
{
  void (*fn) (void *);
  void *fn_data;
  struct gomp_team_state ts;
  struct gomp_task *task;
  struct gomp_thread_pool *thread_pool;
  bool nested;
};

#endif /* __x86_64__ && USE_LITHE */

/* This function is a pthread_create entry point.  This contains the idle
   loop in which a thread waits to be called up to become part of a team.  */

#if __x86_64__ && USE_LITHE
void
gomp_thread_end(lithe_task_t *task, void *arg)
{
/*   free(task->ctx.uc_stack.ss_sp); */
/*   lithe_task_destroy(task); */
/*   free(task); */

  spinlock_lock(&tasks_deque_lock);
  {
    tasks_deque_push(&tasks_deque, task);
  }
  spinlock_unlock(&tasks_deque_lock);

  struct gomp_sched *sched = (struct gomp_sched *) lithe_sched_this();
  __sync_fetch_and_sub(&sched->gomp_threads_to_cleanup, 1);

  gomp_tls_data = NULL;

  lithe_sched_reenter();
}

void
gomp_thread_start (void *arg)
{
  struct gomp_thread *thr = gomp_tls_data = arg;

/*   printf("0x%x begin\n", (unsigned int) (long) pthread_self()); */
  thr->fn(thr->data);
/*   printf("0x%x end\n", (unsigned int) (long) pthread_self()); */

/*   gomp_finish_task(thr->task); */

/*   gomp_barrier_wait (&(thr->ts.team->barrier)); */

  thr->state = ENDED;

  lithe_task_block(gomp_thread_end, NULL);
}
#else
static void *
gomp_thread_start (void *xdata)
{
  struct gomp_thread_start_data *data = xdata;
  struct gomp_thread *thr;
  struct gomp_thread_pool *pool;
  void (*local_fn) (void *);
  void *local_data;

#ifdef HAVE_TLS
  thr = &gomp_tls_data;
#else
  struct gomp_thread local_thr;
  thr = &local_thr;
  pthread_setspecific (gomp_tls_key, thr);
#endif
  gomp_sem_init (&thr->release, 0);

  /* Extract what we need from data.  */
  local_fn = data->fn;
  local_data = data->fn_data;
  thr->thread_pool = data->thread_pool;
  thr->ts = data->ts;
  thr->task = data->task;

  thr->ts.team->ordered_release[thr->ts.team_id] = &thr->release;

  /* Make thread pool local. */
  pool = thr->thread_pool;

  if (data->nested)
    {
      struct gomp_team *team = thr->ts.team;
      struct gomp_task *task = thr->task;

      gomp_barrier_wait (&team->barrier);

      local_fn (local_data);
      gomp_team_barrier_wait (&team->barrier);
      gomp_finish_task (task);
      gomp_barrier_wait_last (&team->barrier);
    }
  else
    {
      pool->threads[thr->ts.team_id] = thr;

      gomp_barrier_wait (&pool->threads_dock);
      do
	{
	  struct gomp_team *team = thr->ts.team;
	  struct gomp_task *task = thr->task;

	  local_fn (local_data);
	  gomp_team_barrier_wait (&team->barrier);
	  gomp_finish_task (task);

	  gomp_barrier_wait (&pool->threads_dock);

	  local_fn = thr->fn;
	  local_data = thr->data;
	  thr->fn = NULL;
	}
      while (local_fn);
    }

  return NULL;
}
#endif /* __x86_64__ && USE_LITHE */


#if __x86_64__ && USE_LITHE
void gomp_enter(void *__this)
{
  struct gomp_sched *sched = (struct gomp_sched *) __this;

/*   printf("0x%x trying here\n", (unsigned int) (long) pthread_self()); */

  if (sched->gomp_threads_can_start) {
    int i = sched->gomp_threads_started;
    if (i < sched->gomp_threads_size)
      i = __sync_fetch_and_add(&sched->gomp_threads_started, 1);

    if (i < sched->gomp_threads_size) {
      assert(sched->gomp_threads[i].state == RUNNABLE);
      sched->gomp_threads[i].state = RUNNING;
      sched->gomp_threads[i].owner = pthread_self();

/*       printf("0x%x working here\n", (unsigned int) (long) pthread_self()); */

      lithe_task_t *task;
      spinlock_lock(&tasks_deque_lock);
      {
	if (tasks_deque_length(&tasks_deque) > 0) {
	  tasks_deque_pop(&tasks_deque, &task);
	} else {
	  task = gomp_malloc(sizeof(lithe_task_t));
	  stack_t stack;
	  stack.ss_size = 4 * (1 << 20);
	  stack.ss_sp = gomp_malloc(stack.ss_size);
	  lithe_task_init(task, &stack);
	}
      }
      spinlock_unlock(&tasks_deque_lock);

      task->tls = NULL;
      started++;
      lithe_task_do(task, gomp_thread_start, &(sched->gomp_threads[i]));
    }
  }

  do {
    /* TODO(benh): Don't require everyone loop-through/share gomp_threads! */
    bool owner = false;
    bool finished = true;
    int i;
    for (i = 0; i < sched->gomp_threads_size; i++) {
      if (sched->gomp_threads[i].owner == pthread_self()) {
	owner = true;
	if (sched->gomp_threads[i].state == RESUMABLE) {
	  sched->gomp_threads[i].state = RUNNING;
	  lithe_task_t *task = sched->gomp_threads[i].lithe_task;
	  sched->gomp_threads[i].lithe_task = NULL;
	  resumed++;
	  lithe_task_resume(task);
	} else if (sched->gomp_threads[i].state != ENDED) {
	  finished = false;
	}
      }
    }

    if (!owner)
      break;

    if (finished)
      break;

/*     printf("0x%x waiting here\n", (unsigned int) (long) pthread_self()); */

/*     gomp_barrier_t *barrier = sched->gomp_threads[0].ts.team->barrier; */
/*     int generation = barrier->generation; */
/*     while (sched->gomp_threads_team_barrier_generation == generation) { */
/*       asm volatile ("rep; nop; mfence" : : : "memory"); */
/*     } */
  } while (sched->gomp_threads_to_cleanup != 1);

  if (sched->gomp_threads[0].owner == pthread_self()) {
    while (coherent_read(sched->gomp_threads_to_cleanup) != 1) {
/*       printf("0x%x (main) waiting here\n", (unsigned int) (long) pthread_self()); */
    }
    if (sched->gomp_threads[0].state == ENDED) {
      sched->gomp_threads[0].state = RUNNING;
      lithe_task_t *task = sched->gomp_threads[0].lithe_task;
      sched->gomp_threads[0].lithe_task = NULL;
/*       printf("0x%x (main) started %d, resumed %d\n", (unsigned int) (long) pthread_self(), started, resumed); */
      lithe_task_resume(task);
    }
/*   } else { */
/*     printf("%p says not the owner of main team member\n", (void *) pthread_self()); */
  }

/*   printf("0x%x started %d, resumed %d\n", (unsigned int) (long) pthread_self(), started, resumed); */
/*   printf("0x%x malloc elapsed time %f\n", (unsigned int) (long) pthread_self(), timer_elapsed()); */
/*   printf("0x%x leaving here\n", (unsigned int) (long) pthread_self()); */
  lithe_sched_yield();
}


void gomp_yield(void *__this, lithe_sched_t *child)
{
  gomp_fatal("unimplemented");
}


void gomp_reg(void *__this, lithe_sched_t *child)
{
  gomp_fatal("unimplemented");
}


void gomp_unreg(void *__this, struct lithe_sched *child)
{
  gomp_fatal("unimplemented");
}


void gomp_request(void *__this, lithe_sched_t *child, int k)
{
  gomp_fatal("unimplemented");
}


void gomp_unblock(void *__this, lithe_task_t *task)
{
  gomp_fatal("unimplemented");
}


const lithe_sched_funcs_t funcs = {
  .enter   = gomp_enter,
  .yield   = gomp_yield,
  .reg     = gomp_reg,
  .unreg   = gomp_unreg,
  .request = gomp_request,
  .unblock = gomp_unblock,
};
#endif /* __x86_64__ && USE_LITHE */


/* Create a new team data structure.  */

struct gomp_team *
gomp_new_team (unsigned nthreads)
{
  struct gomp_team *team;
  size_t size;
  int i;

  size = sizeof (*team) + nthreads * (sizeof (team->ordered_release[0])
				      + sizeof (team->implicit_task[0]));
  team = gomp_malloc (size);

  team->work_share_chunk = 8;
#ifdef HAVE_SYNC_BUILTINS
  team->single_count = 0;
#else
  gomp_mutex_init (&team->work_share_list_free_lock);
#endif
  gomp_init_work_share (&team->work_shares[0], false, nthreads);
  team->work_shares[0].next_alloc = NULL;
  team->work_share_list_free = NULL;
  team->work_share_list_alloc = &team->work_shares[1];
  for (i = 1; i < 7; i++)
    team->work_shares[i].next_free = &team->work_shares[i + 1];
  team->work_shares[i].next_free = NULL;

  team->nthreads = nthreads;
  gomp_barrier_init (&team->barrier, nthreads);

  gomp_sem_init (&team->master_release, 0);
  team->ordered_release = (void *) &team->implicit_task[nthreads];
  team->ordered_release[0] = &team->master_release;

  gomp_mutex_init (&team->task_lock);
  team->task_queue = NULL;
  team->task_count = 0;
  team->task_running_count = 0;

  return team;
}


/* Free a team data structure.  */

static void
free_team (struct gomp_team *team)
{
  gomp_barrier_destroy (&team->barrier);
  gomp_mutex_destroy (&team->task_lock);
  free (team);
}

#if !(__x86_64__ && USE_LITHE)
/* Allocate and initialize a thread pool. */

static struct gomp_thread_pool *gomp_new_thread_pool (void)
{
  struct gomp_thread_pool *pool
    = gomp_malloc (sizeof(struct gomp_thread_pool));
  pool->threads = NULL;
  pool->threads_size = 0;
  pool->threads_used = 0;
  pool->last_team = NULL;
  return pool;
}

static void
gomp_free_pool_helper (void *thread_pool)
{
  struct gomp_thread_pool *pool
    = (struct gomp_thread_pool *) thread_pool;
  gomp_barrier_wait_last (&pool->threads_dock);
  pthread_exit (NULL);
}

/* Free a thread pool and release its threads. */

static void
gomp_free_thread (void *arg __attribute__((unused)))
{
  struct gomp_thread *thr = gomp_thread ();
  struct gomp_thread_pool *pool = thr->thread_pool;
  if (pool)
    {
      if (pool->threads_used > 0)
	{
	  int i;
	  for (i = 1; i < pool->threads_used; i++)
	    {
	      struct gomp_thread *nthr = pool->threads[i];
	      nthr->fn = gomp_free_pool_helper;
	      nthr->data = pool;
	    }
	  /* This barrier undocks threads docked on pool->threads_dock.  */
	  gomp_barrier_wait (&pool->threads_dock);
	  /* And this waits till all threads have called gomp_barrier_wait_last
	     in gomp_free_pool_helper.  */
	  gomp_barrier_wait (&pool->threads_dock);
	  /* Now it is safe to destroy the barrier and free the pool.  */
	  gomp_barrier_destroy (&pool->threads_dock);
	}
      free (pool->threads);
      if (pool->last_team)
	free_team (pool->last_team);
      free (pool);
      thr->thread_pool = NULL;
    }
  if (thr->task != NULL)
    {
      struct gomp_task *task = thr->task;
      gomp_end_task ();
      free (task);
    }
}
#endif /* !(__x86_64__ && USE_LITHE) */

/* Launch a team.  */
#if __x86_64__ && USE_LITHE
static int registered = 0;
#endif /* __x86_64__ && USE_LITHE */

void
gomp_team_start (void (*fn) (void *), void *data, unsigned nthreads,
		 struct gomp_team *team)
#if __x86_64__ && USE_LITHE
{
/*   printf("                  gomp_team_start (%d)\n", nthreads); */
  struct gomp_sched *sched;
  lithe_task_t *task;
  struct gomp_thread *thr, *nthr;
  unsigned i;

  thr = gomp_thread();

  if (thr != NULL) {
    team->prev_ts = thr->ts;
  }

  sched = gomp_malloc(sizeof(struct gomp_sched));

  spinlock_init(&sched->lock);
  sched->gomp_threads_size = nthreads;
  sched->gomp_threads =
    gomp_malloc (sched->gomp_threads_size * sizeof (struct gomp_thread));
  sched->gomp_threads_can_start = false;
  sched->gomp_threads_started = 0;
  sched->gomp_threads_to_cleanup = nthreads;

  task = gomp_malloc(sizeof(lithe_task_t));

  __sync_fetch_and_add(&registered, 1);
  if (lithe_sched_register_task(&funcs, sched, task) != 0) {
    gomp_fatal("lithe_sched_register_task failed: %s", strerror(errno));
  }

  task->tls = thr;

  thr = gomp_tls_data = &(sched->gomp_threads[0]);
  thr->ts.team = team;
  thr->ts.team_id = 0;
  ++thr->ts.level;
  if (nthreads > 1)
    ++thr->ts.active_level;
  thr->ts.work_share = &team->work_shares[0];
  thr->ts.last_work_share = NULL;
#ifdef HAVE_SYNC_BUILTINS
  thr->ts.single_count = 0;
#endif
  thr->ts.static_trip = 0;
/*   thr->task = &team->implicit_task[0]; */
/*   gomp_init_task (thr->task, task, icv); */

  gomp_sem_init (&thr->release, 0);

  thr->state = RUNNING;
/*   printf("owner of main team member is %p\n", (void *) pthread_self()); */
  thr->owner = pthread_self();

  sched->gomp_threads_started = 1;

  if (nthreads == 1) {
    return;
  }

  /* Launch new threads.  */
  for (i = 1; i < nthreads; ++i) {
    nthr = &(sched->gomp_threads[i]);
    nthr->ts.team = team;
    nthr->ts.work_share = &team->work_shares[0];
    nthr->ts.last_work_share = NULL;
    nthr->ts.team_id = i;
    nthr->ts.level = team->prev_ts.level + 1;
    nthr->ts.active_level = thr->ts.active_level;
#ifdef HAVE_SYNC_BUILTINS
    nthr->ts.single_count = 0;
#endif
    nthr->ts.static_trip = 0;
/*     nthr->task = &team->implicit_task[i]; */
/*     gomp_init_task (nthr->task, task, icv); */
    nthr->fn = fn;
    nthr->data = data;
    team->ordered_release[i] = &nthr->release;
    nthr->state = RUNNABLE;
    nthr->owner = (pthread_t) NULL;
  }

  asm volatile ("" : : : "memory");
  sched->gomp_threads_can_start = true;

  if (lithe_sched_request(nthreads - 1) != 0) {
    gomp_fatal("failed to request hard threads");
  }
}
#else
{
  struct gomp_thread_start_data *start_data;
  struct gomp_thread *thr, *nthr;
  struct gomp_task *task;
  struct gomp_task_icv *icv;
  bool nested;
  struct gomp_thread_pool *pool;
  unsigned i, n, old_threads_used = 0;
  pthread_attr_t thread_attr, *attr;

  thr = gomp_thread ();
  nested = thr->ts.team != NULL;
  if (__builtin_expect (thr->thread_pool == NULL, 0))
    {
      thr->thread_pool = gomp_new_thread_pool ();
      pthread_setspecific (gomp_thread_destructor, thr);
    }
  pool = thr->thread_pool;
  task = thr->task;
  icv = task ? &task->icv : &gomp_global_icv;

  /* Always save the previous state, even if this isn't a nested team.
     In particular, we should save any work share state from an outer
     orphaned work share construct.  */
  team->prev_ts = thr->ts;

  thr->ts.team = team;
  thr->ts.team_id = 0;
  ++thr->ts.level;
  if (nthreads > 1)
    ++thr->ts.active_level;
  thr->ts.work_share = &team->work_shares[0];
  thr->ts.last_work_share = NULL;
#ifdef HAVE_SYNC_BUILTINS
  thr->ts.single_count = 0;
#endif
  thr->ts.static_trip = 0;
  thr->task = &team->implicit_task[0];
  gomp_init_task (thr->task, task, icv);

  if (nthreads == 1)
    return;

  i = 1;

  /* We only allow the reuse of idle threads for non-nested PARALLEL
     regions.  This appears to be implied by the semantics of
     threadprivate variables, but perhaps that's reading too much into
     things.  Certainly it does prevent any locking problems, since
     only the initial program thread will modify gomp_threads.  */
  if (!nested)
    {
      old_threads_used = pool->threads_used;

      if (nthreads <= old_threads_used)
	n = nthreads;
      else if (old_threads_used == 0)
	{
	  n = 0;
	  gomp_barrier_init (&pool->threads_dock, nthreads);
	}
      else
	{
	  n = old_threads_used;

	  /* Increase the barrier threshold to make sure all new
	     threads arrive before the team is released.  */
	  gomp_barrier_reinit (&pool->threads_dock, nthreads);
	}

      /* Not true yet, but soon will be.  We're going to release all
	 threads from the dock, and those that aren't part of the
	 team will exit.  */
      pool->threads_used = nthreads;

      /* Release existing idle threads.  */
      for (; i < n; ++i)
	{
	  nthr = pool->threads[i];
	  nthr->ts.team = team;
	  nthr->ts.work_share = &team->work_shares[0];
	  nthr->ts.last_work_share = NULL;
	  nthr->ts.team_id = i;
	  nthr->ts.level = team->prev_ts.level + 1;
	  nthr->ts.active_level = thr->ts.active_level;
#ifdef HAVE_SYNC_BUILTINS
	  nthr->ts.single_count = 0;
#endif
	  nthr->ts.static_trip = 0;
	  nthr->task = &team->implicit_task[i];
	  gomp_init_task (nthr->task, task, icv);
	  nthr->fn = fn;
	  nthr->data = data;
	  team->ordered_release[i] = &nthr->release;
	}

      if (i == nthreads)
	goto do_release;

      /* If necessary, expand the size of the gomp_threads array.  It is
	 expected that changes in the number of threads are rare, thus we
	 make no effort to expand gomp_threads_size geometrically.  */
      if (nthreads >= pool->threads_size)
	{
	  pool->threads_size = nthreads + 1;
	  pool->threads
	    = gomp_realloc (pool->threads,
			    pool->threads_size
			    * sizeof (struct gomp_thread_data *));
	}
    }

  if (__builtin_expect (nthreads > old_threads_used, 0))
    {
      long diff = (long) nthreads - (long) old_threads_used;

      if (old_threads_used == 0)
	--diff;

#ifdef HAVE_SYNC_BUILTINS
      __sync_fetch_and_add (&gomp_managed_threads, diff);
#else
      gomp_mutex_lock (&gomp_remaining_threads_lock);
      gomp_managed_threads += diff;
      gomp_mutex_unlock (&gomp_remaining_threads_lock);
#endif
    }

  attr = &gomp_thread_attr;
  if (__builtin_expect (gomp_cpu_affinity != NULL, 0))
    {
      size_t stacksize;
      pthread_attr_init (&thread_attr);
      pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_DETACHED);
      if (! pthread_attr_getstacksize (&gomp_thread_attr, &stacksize))
	pthread_attr_setstacksize (&thread_attr, stacksize);
      attr = &thread_attr;
    }

  start_data = gomp_alloca (sizeof (struct gomp_thread_start_data)
			    * (nthreads-i));

  /* Launch new threads.  */
  for (; i < nthreads; ++i, ++start_data)
    {
      pthread_t pt;
      int err;

      start_data->fn = fn;
      start_data->fn_data = data;
      start_data->ts.team = team;
      start_data->ts.work_share = &team->work_shares[0];
      start_data->ts.last_work_share = NULL;
      start_data->ts.team_id = i;
      start_data->ts.level = team->prev_ts.level + 1;
      start_data->ts.active_level = thr->ts.active_level;
#ifdef HAVE_SYNC_BUILTINS
      start_data->ts.single_count = 0;
#endif
      start_data->ts.static_trip = 0;
      start_data->task = &team->implicit_task[i];
      gomp_init_task (start_data->task, task, icv);
      start_data->thread_pool = pool;
      start_data->nested = nested;

      if (gomp_cpu_affinity != NULL)
	gomp_init_thread_affinity (attr);

      err = pthread_create (&pt, attr, gomp_thread_start, start_data);
      if (err != 0)
	gomp_fatal ("Thread creation failed: %s", strerror (err));
    }

  if (__builtin_expect (gomp_cpu_affinity != NULL, 0))
    pthread_attr_destroy (&thread_attr);

 do_release:
  gomp_barrier_wait (nested ? &team->barrier : &pool->threads_dock);

  /* Decrease the barrier threshold to match the number of threads
     that should arrive back at the end of this team.  The extra
     threads should be exiting.  Note that we arrange for this test
     to never be true for nested teams.  */
  if (__builtin_expect (nthreads < old_threads_used, 0))
    {
      long diff = (long) nthreads - (long) old_threads_used;

      gomp_barrier_reinit (&pool->threads_dock, nthreads);

#ifdef HAVE_SYNC_BUILTINS
      __sync_fetch_and_add (&gomp_managed_threads, diff);
#else
      gomp_mutex_lock (&gomp_remaining_threads_lock);
      gomp_managed_threads += diff;
      gomp_mutex_unlock (&gomp_remaining_threads_lock);
#endif
    }
}
#endif /* __x86_64__ && USE_LITHE */

#if __x86_64__ && USE_LITHE
void
gomp_team_end_block(lithe_task_t *task, void *arg)
{
  gomp_thread()->lithe_task = task;
  gomp_thread()->state = ENDED;
  lithe_sched_reenter();
}
#endif /* __x86_64__ && USE_LITHE */


/* Terminate the current team.  This is only to be called by the master
   thread.  We assume that we must wait for the other threads.  */

void
gomp_team_end (void)
#if __x86_64__ && USE_LITHE
{
/*   printf("                  gomp_team_end\n"); */
  struct gomp_thread *thr = gomp_thread ();
  struct gomp_team *team = thr->ts.team;

  thr->state = ENDING;

  lithe_task_block(gomp_team_end_block, NULL);

/*   gomp_barrier_wait (&team->barrier); */

  struct gomp_sched *sched;
  lithe_task_t *task;

  sched = (struct gomp_sched *) lithe_sched_this();

  lithe_sched_unregister_task(&task);

  free(sched->gomp_threads);
  free(sched);

/*   gomp_fini_work_share (thr->ts.work_share); */

/*   gomp_end_task (); */

  thr = gomp_tls_data = task->tls;

  if (thr != NULL) {
    thr->ts = team->prev_ts;
  }

  free(task);

  if (__builtin_expect (team->work_shares[0].next_alloc != NULL, 0))
    {
      struct gomp_work_share *ws = team->work_shares[0].next_alloc;
      do
	{
	  struct gomp_work_share *next_ws = ws->next_alloc;
	  free (ws);
	  ws = next_ws;
	}
      while (ws != NULL);
    }
  gomp_sem_destroy (&team->master_release);
#ifndef HAVE_SYNC_BUILTINS
  gomp_mutex_destroy (&team->work_share_list_free_lock);
#endif

  free_team (team);
}
#else
{
  struct gomp_thread *thr = gomp_thread ();
  struct gomp_team *team = thr->ts.team;

  /* This barrier handles all pending explicit threads.  */
  gomp_team_barrier_wait (&team->barrier);
  gomp_fini_work_share (thr->ts.work_share);

  gomp_end_task ();
  thr->ts = team->prev_ts;

  if (__builtin_expect (thr->ts.team != NULL, 0))
    {
#ifdef HAVE_SYNC_BUILTINS
      __sync_fetch_and_add (&gomp_managed_threads, 1L - team->nthreads);
#else
      gomp_mutex_lock (&gomp_remaining_threads_lock);
      gomp_managed_threads -= team->nthreads - 1L;
      gomp_mutex_unlock (&gomp_remaining_threads_lock);
#endif
      /* This barrier has gomp_barrier_wait_last counterparts
	 and ensures the team can be safely destroyed.  */
      gomp_barrier_wait (&team->barrier);
    }

  if (__builtin_expect (team->work_shares[0].next_alloc != NULL, 0))
    {
      struct gomp_work_share *ws = team->work_shares[0].next_alloc;
      do
	{
	  struct gomp_work_share *next_ws = ws->next_alloc;
	  free (ws);
	  ws = next_ws;
	}
      while (ws != NULL);
    }
  gomp_sem_destroy (&team->master_release);
#ifndef HAVE_SYNC_BUILTINS
  gomp_mutex_destroy (&team->work_share_list_free_lock);
#endif

  if (__builtin_expect (thr->ts.team != NULL, 0)
      || __builtin_expect (team->nthreads == 1, 0))
    free_team (team);
  else
    {
      struct gomp_thread_pool *pool = thr->thread_pool;
      if (pool->last_team)
	free_team (pool->last_team);
      pool->last_team = team;
    }
}
#endif /* __x86_64__ && USE_LITHE */


/* Constructors for this file.  */

static void __attribute__((constructor))
initialize_team (void)
{
#if __x86_64__ && USE_LITHE
  spinlock_init(&tasks_deque_lock);
  tasks_deque_init(&tasks_deque);

  int num = get_nprocs();

  int i;
  for (i = 0; i < num; i++) {
    lithe_task_t *task = gomp_malloc(sizeof(lithe_task_t));
    stack_t stack;
    stack.ss_size = 4 * (1 << 20);
    stack.ss_sp = gomp_malloc(stack.ss_size);
    lithe_task_init(task, &stack);
    tasks_deque_push(&tasks_deque, task);
  }
  
#else
  struct gomp_thread *thr;

#ifndef HAVE_TLS
  static struct gomp_thread initial_thread_tls_data;

  pthread_key_create (&gomp_tls_key, NULL);
  pthread_setspecific (gomp_tls_key, &initial_thread_tls_data);
#endif

  if (pthread_key_create (&gomp_thread_destructor, gomp_free_thread) != 0)
    gomp_fatal ("could not create thread pool destructor.");

#ifdef HAVE_TLS
  thr = &gomp_tls_data;
#else
  thr = &initial_thread_tls_data;
#endif
  gomp_sem_init (&thr->release, 0);
#endif /* __x86_64__ && USE_LITHE */
}

static void __attribute__((destructor))
team_destructor (void)
{
  /* Without this dlclose on libgomp could lead to subsequent
     crashes.  */
  pthread_key_delete (gomp_thread_destructor);
#if __x86_64__ && USE_LITHE
/*   printf("registered: %d\n", registered); */
#endif /* __x86_64__ && USE_LITHE */
}

struct gomp_task_icv *
gomp_new_icv (void)
{
  struct gomp_thread *thr = gomp_thread ();
  struct gomp_task *task = gomp_malloc (sizeof (struct gomp_task));
  gomp_init_task (task, NULL, &gomp_global_icv);
  thr->task = task;
  pthread_setspecific (gomp_thread_destructor, thr);
  return &task->icv;
}
