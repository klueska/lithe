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
  task->finished = false;
  task->free_stack = false;

  ssize_t default_stack_size = 4*getpagesize();
  if(attr) {
    if(attr->stack_top) {
      assert(attr->stack_size);
      task->stack_top = attr->stack_top;
      task->stack_size = attr->stack_size;
      return task;
    }
    else {
      if(attr->stack_size)
        task->stack_size = attr->stack_size;
      else
        task->stack_size = default_stack_size;
    }
  }
  else  {
    task->stack_size = default_stack_size;
  }
  task->stack_top = malloc(task->stack_size);
  task->free_stack = true;

  assert(task->stack_top);
  assert(task->stack_size);
  return task;
}

static void __task_yield_default(lithe_sched_t *__this, lithe_task_t *task)
{
}

static void __task_destroy_default(lithe_sched_t *__this, lithe_task_t *task)
{
  assert(task);
  if(task->free_stack) {
    assert(task->stack_top);
    free(task->stack_top);
  }
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
