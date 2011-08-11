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
  friend lithe_task_t* __task_create(lithe_sched_t *__this, lithe_task_attr_t *attr, bool create_stack);
  friend void __task_destroy(lithe_sched_t *__this, lithe_task_t *task, bool free_stack);
  friend void __task_runnable(lithe_sched_t *__this, lithe_task_t *task);
  friend void __task_yield(lithe_sched_t *__this, lithe_task_t *task);
  
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
  virtual lithe_task_t* task_create(lithe_task_attr_t *attr, bool create_stack)
    { return __task_create_default(this, attr, create_stack); }
  virtual void task_destroy(lithe_task_t *task, bool free_stack)
    { return __task_destroy_default(this, task, free_stack); }
  virtual void task_runnable(lithe_task_t *task)
    { return __task_runnable_default(this, task); }
  virtual void task_yield(lithe_task_t *task)
    { return __task_yield_default(this, task); }

 public:
  virtual ~Scheduler() {}
};

}

#endif  // LITHE_SCHED_HH
