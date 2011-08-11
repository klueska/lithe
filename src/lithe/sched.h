/*
 * Lithe Schedulers
 */

#ifndef LITHE_SCHED_H
#define LITHE_SCHED_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <lithe/task.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Declared here, and defined later on */
struct lithe_sched_funcs;
typedef struct lithe_sched_funcs lithe_sched_funcs_t;

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
  void (*child_entered) (lithe_sched_t *__this, lithe_sched_t *child);

  /* Callback to inform of a sucessfully unregistered child. */
  void (*child_exited) (lithe_sched_t *__this, lithe_sched_t *child);

  /* Callback for scheduler specific task creation. */
  lithe_task_t* (*task_create) (lithe_sched_t *__this, void *udata);

  /* Callback for scheduler specific exiting of tasks */
  void (*task_destroy) (lithe_sched_t *__this, lithe_task_t *task);

  /* Function letting this scheduler know that the provided task is runnable.
   * This could result from a blocked task being unblocked, or just after a
   * task is first created and is to be run for the first time */
  void (*task_runnable) (lithe_sched_t *__this, lithe_task_t *task);

  /* Callback for scheduler specific yielding of tasks */
  void (*task_yield) (lithe_sched_t *__this, lithe_task_t *task);

};

#ifdef __cplusplus
}
#endif

#endif  // LITHE_SCHED_H
