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

lithe_task_t* __task_create(lithe_sched_t *__this, lithe_task_attr_t *attr)
{
  return ((Scheduler*)__this)->task_create(attr);
}

void __task_destroy(lithe_sched_t *__this, lithe_task_t *task)
{
  ((Scheduler*)__this)->task_destroy(task);
}

void __task_stack_create(lithe_sched_t *__this, lithe_task_stack_t *stack)
{
  return ((Scheduler*)__this)->task_stack_create(stack);
}

void __task_stack_destroy(lithe_sched_t *__this, lithe_task_stack_t *stack)
{
  ((Scheduler*)__this)->task_stack_destroy(stack);
}

void __task_runnable(lithe_sched_t *__this, lithe_task_t *task)
{
  ((Scheduler*)__this)->task_runnable(task);
}

void __task_yield(lithe_sched_t *__this, lithe_task_t *task)
{
  ((Scheduler*)__this)->task_yield(task);
}

const lithe_sched_funcs_t Scheduler::funcs = {
  /*.vcore_request      = */ __vcore_request,
  /*.vcore_enter        = */ __vcore_enter,
  /*.vcore_return       = */ __vcore_return,
  /*.child_entered      = */ __child_entered,
  /*.child_exited       = */ __child_exited,
  /*.task_create        = */ __task_create,
  /*.task_destroy       = */ __task_destroy,
  /*.task_stack_create  = */ __task_stack_create,
  /*.task_stack_destroy = */ __task_stack_destroy,
  /*.task_runnable      = */ __task_runnable,
  /*.task_yield         = */ __task_yield
};
 
}

