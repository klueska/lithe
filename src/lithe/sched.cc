#include <lithe/sched.hh>
#include <stdlib.h>

namespace lithe {

int __vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k)
{
  return Scheduler::cppcast(__this)->vcore_request(child, k);
}

void __vcore_enter(lithe_sched_t *__this)
{
  Scheduler::cppcast(__this)->vcore_enter();
}

void __vcore_return(lithe_sched_t *__this, lithe_sched_t *child)
{
  Scheduler::cppcast(__this)->vcore_return(child);
}

void __child_entered(lithe_sched_t *__this, lithe_sched_t *child)
{
  Scheduler::cppcast(__this)->child_entered(child);
}

void __child_exited(lithe_sched_t *__this, lithe_sched_t *child)
{
  Scheduler::cppcast(__this)->child_exited(child);
}

lithe_task_t* __task_create(lithe_sched_t *__this, lithe_task_attr_t *attr, bool create_stack)
{
  return Scheduler::cppcast(__this)->task_create(attr, create_stack);
}

void __task_destroy(lithe_sched_t *__this, lithe_task_t *task, bool free_stack)
{
  Scheduler::cppcast(__this)->task_destroy(task, free_stack);
}

void __task_runnable(lithe_sched_t *__this, lithe_task_t *task)
{
  Scheduler::cppcast(__this)->task_runnable(task);
}

void __task_yield(lithe_sched_t *__this, lithe_task_t *task)
{
  Scheduler::cppcast(__this)->task_yield(task);
}

const lithe_sched_funcs_t Scheduler::funcs = {
  /*.vcore_request   = */ __vcore_request,
  /*.vcore_enter     = */ __vcore_enter,
  /*.vcore_return    = */ __vcore_return,
  /*.child_entered   = */ __child_entered,
  /*.child_exited    = */ __child_exited,
  /*.task_create     = */ __task_create,
  /*.task_destroy    = */ __task_destroy,
  /*.task_runnable   = */ __task_runnable,
  /*.task_yield      = */ __task_yield
};
 
}

