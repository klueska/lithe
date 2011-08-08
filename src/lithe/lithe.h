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
#include <lithe/sched.h>
#include <lithe/task.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Value used to inform parent of an unlimited vcore request. */
#define LITHE_REQUEST_MAX (-1)

/**
 * Passes control to a new child scheduler with the specified 'funcs' as
 * callback functions. It hands the current vcore over to this scheduler and
 * runs the start() callback function, passing 'start_arg' as an argument. Once
 * the start() function completes, control is returned back to the parent
 * scheduler and we continue from where this function left off.  Returns -1 if
 * there is an error and sets errno appropriately.
 */
int lithe_sched_start(const lithe_sched_funcs_t *funcs, 
                      void *__sched, void *start_arg);

/**
 * Passes control to a new child scheduler with the specified 'funcs' as
 * callback functions. It hands the current vcore over to this scheduler and
 * then returns. Unlike, lithe_sched_spawn(), this function returns immediately
 * - essentially highjacking the continuation to run inside the child
 * scheduler.  To exit the child, a subsequent call to lithe_sched_exit() is
 * needed. Only at this point will control be passed back to the parent
 * scheduler. Returns -1 if there is an error and sets errno appropriately.
 */
int lithe_sched_enter(const lithe_sched_funcs_t *funcs, void *__sched);

/**
 * Exits the current scheduler, returning control to its parent. Must be paired
 * with a previous call to lithe_sched_enter(). Returns -1 if there is an error
 * and sets errno appropriately.
 */
int lithe_sched_exit();

/**
 * Return a pointer to the current scheduler. I.e. the pointer passed in
 * when the scheduler was started.
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
 * Create a new task with a set of attributes and a start function.  Passing
 * NULL for both func and arg are valid, but require you to subsequently call
 * lithe_task_set_entry() before running the task.  Returns the newly
 * created task on success and NULL on error.
 */
lithe_task_t *lithe_task_create(lithe_task_attr_t *attr, void (*func) (void *), void *arg); 

/* 
 * Destroy an existing task. This task should not be currently running on any
 * vcore.  For currently running tasks, instead use lithe_task_exit().
 */
void lithe_task_destroy(lithe_task_t *task);

/*
 * Initialize a new start function for an existing task. Once the task is
 * restarted it will run from this entry point. 
 */
void lithe_task_set_entry(lithe_task_t *task, void (*func) (void *), void *arg); 

/*
 * Returns the currently executing task.
 */
lithe_task_t *lithe_task_self();

/*
 * Set the task local storage of the current task. 
 */
void lithe_task_settls(void *tls);

/*
 * Get the task local storage of the current task. 
 */
void *lithe_task_gettls();

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
 * Cooperatively yield the current task.
 */
void lithe_task_yield();

/*
 * Exit the current task.
 */
void lithe_task_exit();

#ifdef __cplusplus
}
#endif

#endif  // LITHE_H
