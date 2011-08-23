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

static lithe_context_t* __context_create_default(lithe_sched_t *__this, lithe_context_attr_t *attr)
{
  lithe_context_t *context = (lithe_context_t*)malloc(sizeof(lithe_context_t));
  assert(context);
  return context;
}

static void __context_destroy_default(lithe_sched_t *__this, lithe_context_t *context)
{
  assert(context);
  free(context);
}

static void __context_stack_create_default(lithe_sched_t *__this, lithe_context_stack_t *stack)
{
  if(stack->size == 0)
    stack->size = 4*getpagesize();

  stack->bottom = malloc(stack->size);
  assert(stack->bottom);
}

static void __context_stack_destroy_default(lithe_sched_t *__this, lithe_context_stack_t *stack)
{
  assert(stack->bottom);
  free(stack->bottom);
}

static void __context_runnable_default(lithe_sched_t *__this, lithe_context_t *context)
{
  fatal((char*)"Should not be calling context_runnable()");
}

static void __context_yield_default(lithe_sched_t *__this, lithe_context_t *context)
{
  // Do nothing special by default when a context is yielded
}

#ifdef __cplusplus
}
#endif

#endif
