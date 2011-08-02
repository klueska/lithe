#ifndef LITHE_DEFAULTS_H
#define LITHE_DEFAULTS_H

#include <unistd.h>
#include <lithe/lithe.h>
#include <lithe/fatal.h>

#ifdef __cplusplus
extern "C" {
#endif

static lithe_sched_t *__create_default()
{
  return (lithe_sched_t*)malloc(sizeof(lithe_sched_t));
}

static void __destroy_default(lithe_sched_t *__this)
{
  assert(__this);
  free(__this);
}

static void __start_default(lithe_sched_t *__this)
{
  fatal((char*)"Should not be calling start()");
}

static int __vcore_request_default(lithe_sched_t *__this, lithe_sched_t *child, int k)
{
  return lithe_vcore_request(k);
}

static void __vcore_enter_default(lithe_sched_t *__this)
{
  fatal((char*)"Should not be calling vcore_entry()");
}

static void __vcore_return_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  lithe_vcore_yield();
}

static void __child_started_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  // Do nothing special by default when a child is started
}

static void __child_finished_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  // Do nothing special by default when a child is finished
}

static lithe_task_t* __task_create_default(lithe_sched_t *__this, void *udata)
{
  lithe_task_t *task = (lithe_task_t*)malloc(sizeof(lithe_task_t));
  task->stack.size = 4*getpagesize();
  task->stack.sp = malloc(task->stack.size);
  return task;
}

static void __task_yield_default(lithe_sched_t *__this, lithe_task_t *task)
{
  // Default yield does nothing special, just passes through.
  // Override to cleanup state upon yield
}

static void __task_exit_default(lithe_sched_t *__this, lithe_task_t *task)
{
  assert(task);
  assert(task->stack.sp);
  free(task->stack.sp);
  free(task);
}

static void __task_runnable_default(lithe_sched_t *__this, lithe_task_t *task)
{
  fatal((char*)"Should not be calling task_runnable()");
}

#ifdef __cplusplus
}
#endif

#endif
