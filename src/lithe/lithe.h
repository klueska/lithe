/*
 * Lithe
 *
 * TODO: Include a discussion of the transition stack and
 * callbacks versus entry points.
 *
 * TODO: Include a discussion of tasks, including the
 * distinction between implicit and explicit tasks.
 */

#ifndef LITHE_H
#define LITHE_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <stdarg.h>
#include <vcore.h>
#include <uthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Value used to inform parent of an unlimited vcore request. */
#define LITHE_REQUEST_MAX (-1)

/* Declared here, and defined later on */
struct lithe_sched_funcs;
typedef struct lithe_sched_funcs lithe_sched_funcs_t;
struct lithe_task;
typedef struct lithe_task lithe_task_t;

/* Opaque type required by every lithe scheduler, but used only internally by
 * lithe itself */
struct lithe_sched_idata;
typedef struct lithe_sched_idata lithe_sched_idata_t;

/* Basic lithe scheduler structure. All derived schedulers MUST have this as
 * their first field so that they can be cast properly within lithe */
struct lithe_sched {
  /* Scheduler functions. */
  const lithe_sched_funcs_t *funcs;

  /* Scheduler state, managed internally by lithe */
  lithe_sched_idata_t *idata;
};
typedef struct lithe_sched lithe_sched_t;


/* Lithe scheduler callbacks/entrypoints. */
struct lithe_sched_funcs {
  /* Initialization function called when this scheduler is first registered */
  lithe_sched_t* (*construct) (void *__sched);

  /* Destructor function called just before this scheduler is deregistered */
  void (*destroy) (lithe_sched_t *__this);

  /* Start function of the scheduler.  From here you can launch tasks, do whatever */
  void (*start) (lithe_sched_t *__this);

  /* Function ultimately responsible for granting vcore requests from a child
   * scheduler. This function is automatically called when a child invokes
   * lithe_sched_request() from within it's current scheduler */
  int (*vcore_request) (lithe_sched_t *__this, lithe_sched_t *child, int k);

  /* Entry point for vcores granted to this scheduler by a call to
   * lithe_vcore_request() */
  void (*vcore_enter) (lithe_sched_t *__this);

  /* Entry point for vcores given back to this scheduler by a call to
   * lithe_vcore_yield() */
  void (*vcore_return) (lithe_sched_t *__this, lithe_sched_t *child);

  /* Callback to inform of a successfully registered child. */
  void (*child_started) (lithe_sched_t *__this, lithe_sched_t *child);

  /* Callback to inform of a sucessfully unregistered child. */
  void (*child_finished) (lithe_sched_t *__this, lithe_sched_t *child);

  /* Callback for scheduler specific task creation. */
  lithe_task_t* (*task_create) (lithe_sched_t *__this, void *udata);

  /* Callback for scheduler specific yielding of tasks */
  void (*task_yield) (lithe_sched_t *__this, lithe_task_t *task);

  /* Callback for scheduler specific exiting of tasks */
  void (*task_exit) (lithe_sched_t *__this, lithe_task_t *task);

  /* Function letting this scheduler know that the provided task is runnable.
   * This could result from a blocked task being unblocked, or just after a
   * task is first created and is to be run for the first time */
  void (*task_runnable) (lithe_sched_t *__this, lithe_task_t *task);

};

typedef struct {
  /* Stack size for the lithe task */
  ssize_t stack_size;

} lithe_task_attr_t;
  
/* Basic lithe task structure.  All derived scheduler tasks MUST have this as
 * their first field so that they can be cast properly within the lithe
 * scheduler. */
struct lithe_task {
  /* Userlevel thread context. */
  uthread_t uth;

  /* Start function for the task */
  void (*start_func) (void *);

  /* Argument for the start function */
  void *arg;

  /* Size of the task's stack */
  size_t stack_size;

  /* Pointer to the task's stack */
  void *sp;
};

/**
 * Registers the scheduler functions passed as a parameter to create a new
 * lithe scheduler.  It also hands the current vcore over to this scheduler
 * until it is unregistered later on.  Once unregistered this call will
 * complete.  Returns -1 if there is an error and sets errno appropriately.
 */
int lithe_sched_start(const lithe_sched_funcs_t *funcs, void *arg);

/**
 * Return the a pointer to the current scheduler. I.e. the pointer passed in
 * when the scheduler was registered 
 */
void *lithe_sched_current();

/**
 * Request the specified number of vcores from the parent. Note that the parent
 * is free to make a request using the calling vcore to their parent if
 * necessary. Returns the number of vcores actually granted.
 */
int lithe_vcore_request(int k);

/**
 * Grant current vcore to specified scheduler. The specified
 * scheduler must be non-null and a registered scheduler of the
 * current scheduler. This function never returns unless child is
 * NULL, in which case it sets errno appropriately and returns -1.
 */
int lithe_vcore_grant(lithe_sched_t *child);

/**
 * Yield current vcore to parent scheduler. This function should
 * never return.
 */
void lithe_vcore_yield();

/*
 * Initialize a new task. Returns the newly initialized task on success and
 * NULL on error.
 */
lithe_task_t *lithe_task_create(lithe_task_attr_t *attr, void (*func) (void *), void *arg); 

/*
 * Returns the currently executing task.
 */
lithe_task_t *lithe_task_self();

/*
 * Run the specified task.  It must have been precreated. This 
 * function never returns on success and returns -1 on error and sets
 * errno appropriately.
 */
int lithe_task_run(lithe_task_t *task);

/**
 * Invoke the specified function with the current task. Note that you
 * can not block without a valid task (i.e. you can not block when
 * executing in vcore context). Returns 0 on success (after the task
 * has been resumed) and -1 on error and sets errno appropriately.
*/
int lithe_task_block(void (*func) (lithe_task_t *, void *), void *arg);

/**
 * Notifies the current scheduler that the specified task is
 * resumable. Returns 0 on success and -1 on error and sets errno
 * appropriately.
 */
int lithe_task_unblock(lithe_task_t *task);

/**
 * Invoke the specified function with the current task. Note that you
 * can not block without a valid task (i.e. you can not block when
 * executing in vcore context). Returns 0 on success (after the task
 * has been resumed) and -1 on error and sets errno appropriately.
*/
void lithe_task_yield();

/*
 * Exit an existing task, freeing all of its data structures and meta data in
 * the process. Returns 0 on success and -1 on error and sets errno
 * appropriately.
 */
void lithe_task_exit();

#ifdef __cplusplus
}
#endif

#endif  // LITHE_H
