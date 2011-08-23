#include <lithe/sched.hh>
#include <stdlib.h>

namespace lithe {

int __vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k)
{
  return ((Scheduler*)__this)->vcore_request(child, k);
}

void __vcore_enter(lithe_sched_t *__this)
{
  ((Scheduler*)__this)->vcore_enter();
}

void __vcore_return(lithe_sched_t *__this, lithe_sched_t *child)
{
  ((Scheduler*)__this)->vcore_return(child);
}

void __child_entered(lithe_sched_t *__this, lithe_sched_t *child)
{
  ((Scheduler*)__this)->child_entered(child);
}

void __child_exited(lithe_sched_t *__this, lithe_sched_t *child)
{
  ((Scheduler*)__this)->child_exited(child);
}

lithe_context_t* __context_create(lithe_sched_t *__this, lithe_context_attr_t *attr)
{
  return ((Scheduler*)__this)->context_create(attr);
}

void __context_destroy(lithe_sched_t *__this, lithe_context_t *context)
{
  ((Scheduler*)__this)->context_destroy(context);
}

void __context_stack_create(lithe_sched_t *__this, lithe_context_stack_t *stack)
{
  return ((Scheduler*)__this)->context_stack_create(stack);
}

void __context_stack_destroy(lithe_sched_t *__this, lithe_context_stack_t *stack)
{
  ((Scheduler*)__this)->context_stack_destroy(stack);
}

void __context_runnable(lithe_sched_t *__this, lithe_context_t *context)
{
  ((Scheduler*)__this)->context_runnable(context);
}

void __context_yield(lithe_sched_t *__this, lithe_context_t *context)
{
  ((Scheduler*)__this)->context_yield(context);
}

const lithe_sched_funcs_t Scheduler::funcs = {
  /*.vcore_request         = */ __vcore_request,
  /*.vcore_enter           = */ __vcore_enter,
  /*.vcore_return          = */ __vcore_return,
  /*.child_entered         = */ __child_entered,
  /*.child_exited          = */ __child_exited,
  /*.context_create        = */ __context_create,
  /*.context_destroy       = */ __context_destroy,
  /*.context_stack_create  = */ __context_stack_create,
  /*.context_stack_destroy = */ __context_stack_destroy,
  /*.context_runnable      = */ __context_runnable,
  /*.context_yield         = */ __context_yield
};
 
}

