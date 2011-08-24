#ifndef LITHE_SCHED_HH
#define LITHE_SCHED_HH

#include <lithe/sched.h>
#include <lithe/defaults.h>

namespace lithe {

class Scheduler;

class Scheduler : public lithe_sched_t {
 protected:
  static const lithe_sched_funcs_t static_funcs;

 private:
  friend int __vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k);
  friend void __vcore_enter(lithe_sched_t *__this);
  friend void __vcore_return(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __child_entered(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __child_exited(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __context_block(lithe_sched_t *__this, lithe_context_t *context);
  friend void __context_unblock(lithe_sched_t *__this, lithe_context_t *context);
  friend void __context_yield(lithe_sched_t *__this, lithe_context_t *context);
  friend void __context_exit(lithe_sched_t *__this, lithe_context_t *context);
  
 protected:
  virtual void vcore_enter() = 0;

  virtual int vcore_request(lithe_sched_t *child, int k) 
    { return __vcore_request_default(this, child, k); }
  virtual void vcore_return(lithe_sched_t *child)
    { return __vcore_return_default(this, child); }
  virtual void child_entered(lithe_sched_t *child)
    { return __child_entered_default(this, child); }
  virtual void child_exited(lithe_sched_t *child)
    { return __child_exited_default(this, child); }
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
