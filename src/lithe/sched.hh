#ifndef LITHE_SCHED_HH
#define LITHE_SCHED_HH

#include <lithe/sched.h>
#include <lithe/defaults.h>

namespace lithe {

class Scheduler;

class Scheduler : public lithe_sched_t {
 public:
  static const lithe_sched_funcs_t funcs;

 private:
  friend int __vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k);
  friend void __vcore_enter(lithe_sched_t *__this);
  friend void __vcore_return(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __child_entered(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __child_exited(lithe_sched_t *__this, lithe_sched_t *child);
  friend lithe_context_t* __context_create(lithe_sched_t *__this, lithe_context_attr_t *attr);
  friend void __context_destroy(lithe_sched_t *__this, lithe_context_t *context);
  friend void __context_stack_create(lithe_sched_t *__this, lithe_context_stack_t *stack);
  friend void __context_stack_destroy(lithe_sched_t *__this, lithe_context_stack_t *stack);
  friend void __context_runnable(lithe_sched_t *__this, lithe_context_t *context);
  friend void __context_yield(lithe_sched_t *__this, lithe_context_t *context);
  
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
  virtual lithe_context_t* context_create(lithe_context_attr_t *attr)
    { return __context_create_default(this, attr); }
  virtual void context_destroy(lithe_context_t *context)
    { return __context_destroy_default(this, context); }
  virtual void context_stack_create(lithe_context_stack_t *stack)
    { return __context_stack_create_default(this, stack); }
  virtual void context_stack_destroy(lithe_context_stack_t *stack)
    { return __context_stack_destroy_default(this, stack); }
  virtual void context_runnable(lithe_context_t *context)
    { return __context_runnable_default(this, context); }
  virtual void context_yield(lithe_context_t *context)
    { return __context_yield_default(this, context); }

 public:
  virtual ~Scheduler() {}
};

}

#endif  // LITHE_SCHED_HH
