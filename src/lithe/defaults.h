#ifndef LITHE_DEFAULTS_H
#define LITHE_DEFAULTS_H

#include <unistd.h>
#include <lithe/lithe.h>
#include <lithe/fatal.h>

lithe_sched_t *__create_default()
{
  return (lithe_sched_t*)malloc(sizeof(lithe_sched_t));
}

void __destroy_default(lithe_sched_t *__this)
{
  assert(__this);
  free(__this);
}

void __start_default(lithe_sched_t *__this)
{
  fatal("Should not be calling start()");
}

int __vcore_request_default(lithe_sched_t *__this, lithe_sched_t *child, int k)
{
  return lithe_vcore_request(k);
}

void __vcore_enter_default(lithe_sched_t *__this)
{
  fatal("Should not be calling vcore_entry()");
}

void __vcore_return_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  lithe_vcore_yield();
}

void __child_started_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  // Do nothing special by default when a child is started
}

void __child_finished_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  // Do nothing special by default when a child is finished
}

lithe_task_t* __task_create_default(lithe_sched_t *__this, void *udata)
{
  lithe_task_t *task = (lithe_task_t*)malloc(sizeof(lithe_task_t));
  task->stack.size = 4*getpagesize();
  task->stack.sp = malloc(task->stack.size);
  return task;
}

void __task_yield_default(lithe_sched_t *__this, lithe_task_t *task)
{
  // Default yield does nothing special, just passes through.
  // Override to cleanup state upon yield
}

void __task_exit_default(lithe_sched_t *__this, lithe_task_t *task)
{
  assert(task);
  assert(task->stack.sp);
  free(task->stack.sp);
  free(task);
}

void __task_runnable_default(lithe_sched_t *__this, lithe_task_t *task)
{
  fatal("Should not be calling task_runnable()");
}

#endif
