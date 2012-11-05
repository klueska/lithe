/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/*
 * Lithe API
 */

#ifndef LITHE_H
#define LITHE_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <stdarg.h>
#include <parlib/parlib.h>
#include "sched.h"
#include "context.h"
#include "hart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Passes control to a new child scheduler. The 'funcs' field of the scheduler
 * must already be set up properly or the call will fail. It hands the current
 * hart over to this scheduler and then returns. To exit the child, a
 * subsequent call to lithe_sched_exit() is needed. Only at this point will
 * control be passed back to the parent scheduler. Returns -1 if there is an
 * error and sets errno appropriately.
 */
int lithe_sched_enter(lithe_sched_t *child);

/**
 * Exits the current scheduler, returning control to its parent. Must be paired
 * with a previous call to lithe_sched_enter(). Returns -1 if there is an error
 * and sets errno appropriately.
 */
int lithe_sched_exit();

/**
 * Return a pointer to the current scheduler. I.e. the pointer passed in
 * when the scheduler was entered.
 */
lithe_sched_t *lithe_sched_current();

/**
 * Request a specified number of harts from the parent. Note that the parent
 * is free to make a request using the calling hart to their own parent if
 * necessary. These harts will trickle in over time as they are granted to the
 * requesting scheduler. Returns 0 on success and -1 on error. 
 */
int lithe_hart_request(int k);

/**
 * Grant the current hart to another scheduler.  Triggered by a previous call
 * to lithe_hart_request() by a child scheduler. This function never returns.
 * The 'unlock_func()' and its corresponding 'lock' parameter are passed in by
 * a parent scheduler so that the lithe runtime can synchronize references to
 * the child scheduler in the rare case that the child scheduler may currently
 * be in the process of exiting. For example, the unlock function passed in
 * should be the same one used to add/remove the child scheduler from a list in
 * the parent scheduler.
 */
void lithe_hart_grant(lithe_sched_t *child, void (*unlock_func) (void *), void *lock);

/**
 * Yield current hart to parent scheduler. This function should
 * never return.
 */
void lithe_hart_yield();

/*
 * Initialize the proper lithe internal state for an existing context. The
 * context parameter MUST already contain a valid stack pointer and stack size.
 * This function MUST be paired with a call to lithe_context_cleanup() in order
 * to properly cleanup the lithe internal state once the context is no longer
 * usable.  To reinitialize a context with a new start function without calling
 * lithe_context_cleanup() first, use lithe_context_reinit() instead.  Once the
 * context is restarted it will run from the entry point specified.
 */
void lithe_context_init(lithe_context_t *context, void (*func) (void *), void *arg);

/*
 * Used to reinitialize the lithe internal state for a context already
 * initialized via lithe_context_cleanup().  Normally each call to
 * lithe_context_init() must be paired with a call to lithe_context_cleanup()
 * before the context can be reused for anything. The lithe_context_reinit()
 * function allows you to reinitialize this context with a new start function
 * any number of times before pairing it with lithe_context_cleanup(). Once the
 * context is restarted it will run from the entry point specified.
 */
void lithe_context_reinit(lithe_context_t *context, void (*func) (void *), void *arg);

/* 
 * Cleanup an existing context. This context should NOT currently be running on
 * any hart, though this is not enforced by the lithe runtime.
 */
void lithe_context_cleanup(lithe_context_t *context);

/*
 * Returns a pointer to the currently executing context.
 */
lithe_context_t *lithe_context_self();

/*
 * Run the specified context.  MUST only be run from hart context. Upon
 * completion, the context is yielded, and must be either retasked for other
 * use via lithe_context_reinit() or cleaned up via a call to
 * lithe_context_cleanup().  This function never returns on success and returns
 * -1 on error and sets errno appropriately.
 */
int lithe_context_run(lithe_context_t *context);

/**
 * Invoke the specified function with the current context and block that
 * context. This function is useful for blocking a context and managing when to
 * unblock it by some component other than the scheduler associated with this
 * context.  The scheduler will recieve a callback notifying it that this
 * context has blocked and should not be run. The scheduler will receive
 * another callback later to notify it when this context has unblocked and can
 * once again be resumed. Returns 0 on success (after the context has been
 * resumed) and -1 on error and sets errno appropriately.
*/
int lithe_context_block(void (*func) (lithe_context_t *, void *), void *arg);

/**
 * Notifies the current scheduler that the specified context is now resumable.
 * This chould only be called on contexts previously blocked via a call to
 * lithe_context_block(). Returns 0 on success and -1 on error and sets errno
 * appropriately.
 */
int lithe_context_unblock(lithe_context_t *context);

/**
 * Cooperatively yield the current context to the current scheduler.  The
 * scheduler receives a callback notifiying it that the context has yielded and
 * can decide from there when to resume it.
 */
void lithe_context_yield();

#ifdef __cplusplus
}
#endif

#endif  // LITHE_H
