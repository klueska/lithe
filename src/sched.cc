/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

#include <stdlib.h>
#include "sched.hh"

namespace lithe {

void __hart_request(lithe_sched_t *__this, lithe_sched_t *child, size_t h)
{
  ((Scheduler*)__this)->hart_request(child, h);
}

void __hart_enter(lithe_sched_t *__this)
{
  ((Scheduler*)__this)->hart_enter();
}

void __hart_return(lithe_sched_t *__this, lithe_sched_t *child)
{
  ((Scheduler*)__this)->hart_return(child);
}

void __sched_enter(lithe_sched_t *__this)
{
  ((Scheduler*)__this)->sched_enter();
}

void __sched_exit(lithe_sched_t *__this)
{
  ((Scheduler*)__this)->sched_exit();
}

void __child_enter(lithe_sched_t *__this, lithe_sched_t *child)
{
  ((Scheduler*)__this)->child_enter(child);
}

void __child_exit(lithe_sched_t *__this, lithe_sched_t *child)
{
  ((Scheduler*)__this)->child_exit(child);
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

const lithe_sched_funcs_t Scheduler::static_funcs = {
  /*.hart_request          = */ __hart_request,
  /*.hart_enter            = */ __hart_enter,
  /*.hart_return           = */ __hart_return,
  /*.sched_enter           = */ __sched_enter,
  /*.sched_exit            = */ __sched_exit,
  /*.child_enter           = */ __child_enter,
  /*.child_exit            = */ __child_exit,
  /*.context_block         = */ __context_block,
  /*.context_unblock       = */ __context_unblock,
  /*.context_yield         = */ __context_yield,
  /*.context_exit          = */ __context_exit
};
 
}

