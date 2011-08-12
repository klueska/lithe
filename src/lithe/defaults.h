#ifndef LITHE_DEFAULTS_H
#define LITHE_DEFAULTS_H

#include <unistd.h>
#include <lithe/lithe.h>
#include <lithe/fatal.h>

#ifdef __cplusplus
extern "C" {
#endif

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

static void __child_entered_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  // Do nothing special by default when a child is entered
}

static void __child_exited_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  // Do nothing special by default when a child is exited
}

static lithe_task_t* __task_create_default(lithe_sched_t *__this, lithe_task_attr_t *attr)
{
  lithe_task_t *task = (lithe_task_t*)malloc(sizeof(lithe_task_t));
  assert(task);
  return task;
}

static void __task_destroy_default(lithe_sched_t *__this, lithe_task_t *task)
{
  assert(task);
  free(task);
}

static void __task_stack_create_default(lithe_sched_t *__this, lithe_task_stack_t *stack)
{
  if(stack->size == 0)
    stack->size = 4*getpagesize();

  stack->bottom = malloc(stack->size);
  assert(stack->bottom);
}

static void __task_stack_destroy_default(lithe_sched_t *__this, lithe_task_stack_t *stack)
{
  assert(stack->bottom);
  free(stack->bottom);
}

static void __task_runnable_default(lithe_sched_t *__this, lithe_task_t *task)
{
  fatal((char*)"Should not be calling task_runnable()");
}

static void __task_yield_default(lithe_sched_t *__this, lithe_task_t *task)
{
  // Do nothing special by default when a task is yielded
}

#ifdef __cplusplus
}
#endif

#endif
