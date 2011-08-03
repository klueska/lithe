#ifndef LITHE_HH
#define LITHE_HH

#include <lithe/lithe.h>
#include <lithe/defaults.h>

namespace lithe {

class Scheduler;

extern "C" {
typedef struct lithe_sched_cxx  {
  lithe_sched_t sched;
  Scheduler *__this;
} lithe_sched_cxx_t;
}

class Scheduler {
 private:
  friend lithe_sched_t* __create(void *arg);
  friend void __destroy(lithe_sched_t *__this);
  friend void __start(lithe_sched_t *__this, void *arg);
  friend int __vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k);
  friend void __vcore_enter(lithe_sched_t *__this);
  friend void __vcore_return(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __child_started(lithe_sched_t *__this, lithe_sched_t *child);
  friend void __child_finished(lithe_sched_t *__this, lithe_sched_t *child);
  friend lithe_task_t* __task_create(lithe_sched_t *__this, void *udata);
  friend void __task_yield(lithe_sched_t *__this, lithe_task_t *task);
  friend void __task_exit(lithe_sched_t *__this, lithe_task_t *task);
  friend void __task_runnable(lithe_sched_t *__this, lithe_task_t *task);
  
 protected:
  virtual void start(void *arg) = 0;
  virtual void vcore_enter() = 0;

  virtual int vcore_request(lithe_sched_t *child, int k) 
    { return __vcore_request_default((lithe_sched_t*)&csched, child, k); }
  virtual void vcore_return(lithe_sched_t *child)
    { return __vcore_return_default((lithe_sched_t*)&csched, child); }
  virtual void child_started(lithe_sched_t *child)
    { return __child_started_default((lithe_sched_t*)&csched, child); }
  virtual void child_finished(lithe_sched_t *child)
    { return __child_finished_default((lithe_sched_t*)&csched, child); }
  virtual lithe_task_t* task_create(void *udata)
    { return __task_create_default((lithe_sched_t*)&csched, udata); }
  virtual void task_yield(lithe_task_t *task)
    { return __task_yield_default((lithe_sched_t*)&csched, task); }
  virtual void task_exit(lithe_task_t *task)
    { return __task_exit_default((lithe_sched_t*)&csched, task); }
  virtual void task_runnable(lithe_task_t *task)
    { return __task_runnable_default((lithe_sched_t*)&csched, task); }
  
 public:
  lithe_sched_cxx_t csched;
  static const lithe_sched_funcs_t funcs;
};

}

#endif  // LITHE_HH
