/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

#ifndef LITHE_SCHED_HH
#define LITHE_SCHED_HH

#include "sched.h"
#include "defaults.h"

namespace lithe {

class Scheduler : public lithe_sched_t {
 protected:
  static const lithe_sched_funcs_t static_funcs;

 private:
  friend void __hart_request(lithe_sched_t *__this, lithe_sched_t *child, size_t h);
  friend void __hart_enter(lithe_sched_t *__this);
  friend void __hart_return(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __sched_enter(lithe_sched_t *__this);
  friend void __sched_exit(lithe_sched_t *__this);
  friend void __child_enter(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __child_exit(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __context_block(lithe_sched_t *__this, lithe_context_t *context);
  friend void __context_unblock(lithe_sched_t *__this, lithe_context_t *context);
  friend void __context_yield(lithe_sched_t *__this, lithe_context_t *context);
  friend void __context_exit(lithe_sched_t *__this, lithe_context_t *context);
  
 protected:
  virtual void hart_enter() = 0;

  virtual void hart_request(lithe_sched_t *child, size_t h)
    { return __hart_request_default(this, child, h); }
  virtual void hart_return(lithe_sched_t *child)
    { return __hart_return_default(this, child); }
  virtual void sched_enter()
    { return __sched_enter_default(this); }
  virtual void sched_exit()
    { return __sched_exit_default(this); }
  virtual void child_enter(lithe_sched_t *child)
    { return __child_enter_default(this, child); }
  virtual void child_exit(lithe_sched_t *child)
    { return __child_exit_default(this, child); }
  virtual void context_block(lithe_context_t *context)
    { return __context_block_default(this, context); }
  virtual void context_unblock(lithe_context_t *context)
    { return __context_unblock_default(this, context); }
  virtual void context_yield(lithe_context_t *context)
    { return __context_yield_default(this, context); }
  virtual void context_exit(lithe_context_t *context)
    { return __context_exit_default(this, context); }

 public:
  Scheduler() {funcs = &Scheduler::static_funcs; }
  virtual ~Scheduler() {}
};

}

#endif  // LITHE_SCHED_HH
