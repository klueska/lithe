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

void __context_block(lithe_sched_t *__this, lithe_context_t *context)
{
  ((Scheduler*)__this)->context_block(context);
}

void __context_unblock(lithe_sched_t *__this, lithe_context_t *context)
{
  ((Scheduler*)__this)->context_unblock(context);
}

void __context_yield(lithe_sched_t *__this, lithe_context_t *context)
{
  ((Scheduler*)__this)->context_yield(context);
}

void __context_exit(lithe_sched_t *__this, lithe_context_t *context)
{
  ((Scheduler*)__this)->context_exit(context);
}

const lithe_sched_funcs_t Scheduler::funcs = {
  /*.vcore_request         = */ __vcore_request,
  /*.vcore_enter           = */ __vcore_enter,
  /*.vcore_return          = */ __vcore_return,
  /*.child_entered         = */ __child_entered,
  /*.child_exited          = */ __child_exited,
  /*.context_block         = */ __context_block,
  /*.context_unblock       = */ __context_unblock,
  /*.context_yield         = */ __context_yield,
  /*.context_exit          = */ __context_exit
};
 
}

