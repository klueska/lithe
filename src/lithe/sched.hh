#ifndef LITHE_SCHED_HH
#define LITHE_SCHED_HH

#include <lithe/sched.h>
#include <lithe/defaults.h>

namespace lithe {

class Scheduler;

extern "C" {
typedef struct {
  lithe_sched_t csched;
  Scheduler *cppsched;
} lithe_sched_cpp_t;
}

class Scheduler {
 private:
  lithe_sched_cpp_t icsched;
  lithe_sched_t *csched;
 public:
  static const lithe_sched_funcs_t funcs;

 private:
  friend int __vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k);
  friend void __vcore_enter(lithe_sched_t *__this);
  friend void __vcore_return(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __child_started(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __child_finished(lithe_sched_t *__this, lithe_sched_t *child);
  friend lithe_task_t* __task_create(lithe_sched_t *__this, void *udata);
  friend void __task_destroy(lithe_sched_t *__this, lithe_task_t *task);
  friend void __task_runnable(lithe_sched_t *__this, lithe_task_t *task);
  friend void __task_yield(lithe_sched_t *__this, lithe_task_t *task);
  
 protected:
  virtual void vcore_enter() = 0;

  virtual int vcore_request(lithe_sched_t *child, int k) 
    { return __vcore_request_default(csched, child, k); }
  virtual void vcore_return(lithe_sched_t *child)
    { return __vcore_return_default(csched, child); }
  virtual void child_started(lithe_sched_t *child)
    { return __child_started_default(csched, child); }
  virtual void child_finished(lithe_sched_t *child)
    { return __child_finished_default(csched, child); }
  virtual lithe_task_t* task_create(void *udata)
    { return __task_create_default(csched, udata); }
  virtual void task_yield(lithe_task_t *task)
    { return __task_yield_default(csched, task); }
  virtual void task_destroy(lithe_task_t *task)
    { return __task_destroy_default(csched, task); }
  virtual void task_runnable(lithe_task_t *task)
    { return __task_runnable_default(csched, task); }

 public:
  Scheduler() 
  { 
    icsched.cppsched = this;
    csched = &icsched.csched;
  }
  virtual ~Scheduler() {}

  static Scheduler *cppcast(lithe_sched_t *sched)
  {
    return ((lithe_sched_cpp_t*)sched)->cppsched;
  }

  static lithe_sched_t *ccast(Scheduler *sched)
  {
    return sched->csched;
  }
};

}

#endif  // LITHE_SCHED_HH
