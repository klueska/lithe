/*
 * Lithe
 *
 * TODO: Include a discussion of the transition stack and
 * callbacks versus entry points.
 *
 * TODO: Include a discussion of contexts, including the
 * distinction between implicit and explicit contexts.
 */

#ifndef LITHE_H
#define LITHE_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <stdarg.h>
#include <lithe/sched.h>
#include <lithe/context.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Value used to inform parent of an unlimited vcore request. */
#define LITHE_REQUEST_MAX (-1)

/**
 * Passes control to a new child scheduler with the specified 'funcs' as
 * callback functions. It hands the current vcore over to this scheduler and
 * then returns. To exit the child, a subsequent call to lithe_sched_exit() is
 * needed. Only at this point will control be passed back to the parent
 * scheduler. Returns -1 if there is an error and sets errno appropriately.
 */
int lithe_sched_enter(const lithe_sched_funcs_t *funcs, lithe_sched_t *child);

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
lithe_sched_t *lithe_sched_current();

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
 * Initialize the attributes used to create / initialize a lithe context. Must be
 * called on all attribute variables before passing them to either
 * lithe_context_create() or lithe_context_init(). 
 */
void lithe_context_attr_init(lithe_context_attr_t *attr);
/*
 * Create a new context with a set of attributes and a start function.  Passing
 * NULL for both func and arg are valid, but require you to subsequently call
 * lithe_context_init() before running the context.  Returns the newly
 * created context on success and NULL on error.
 */
lithe_context_t *lithe_context_create(lithe_context_attr_t *attr, void (*func) (void *), void *arg); 

/*
 * Initialize a new state for an existing context. The attr parameter MUST contain
 * a valid stack pointer and stack size. Once the context is restarted it will run
 * from the entry point specified.
 */
void lithe_context_init(lithe_context_t *context, lithe_context_attr_t *attr, void (*func) (void *), void *arg);

/* 
 * Destroy an existing context. This context should not be currently running on any
 * vcore.  For currently running contexts, instead use lithe_context_exit().
 */
void lithe_context_destroy(lithe_context_t *context);

/*
 * Returns the currently executing context.
 */
lithe_context_t *lithe_context_self();

/*
 * Set the context local storage of the current context. 
 */
void lithe_context_settls(void *tls);

/*
 * Get the context local storage of the current context. 
 */
void *lithe_context_gettls();

/*
 * Run the specified context.  Upon completion, the context is yielded, and must be
 * explicitly destroyed via a call to lithe_context_detroy() to free any memory
 * associated with it. It must have been precreated. This function never
 * returns on success and returns -1 on error and sets errno appropriately.
 */
int lithe_context_run(lithe_context_t *context);

/**
 * Invoke the specified function with the current context. Note that you
 * can not block without a valid context (i.e. you can not block when
 * executing in vcore context). Returns 0 on success (after the context
 * has been resumed) and -1 on error and sets errno appropriately.
*/
int lithe_context_block(void (*func) (lithe_context_t *, void *), void *arg);

/**
 * Notifies the current scheduler that the specified context is
 * resumable. Returns 0 on success and -1 on error and sets errno
 * appropriately.
 */
int lithe_context_unblock(lithe_context_t *context);

/**
 * Cooperatively yield the current context.
 */
void lithe_context_yield();

/*
 * Exit the current context.
 */
void lithe_context_exit();

#ifdef __cplusplus
}
#endif

#endif  // LITHE_H
