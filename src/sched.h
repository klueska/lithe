/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/*
 * Lithe Schedulers
 */

#ifndef LITHE_SCHED_H
#define LITHE_SCHED_H

#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lithe sched type passed around between the lithe runtime and 2nd-level
 * schedulers */
struct lithe_sched;
typedef struct lithe_sched lithe_sched_t;

/* Lithe scheduler callbacks/entrypoints. */
typedef struct lithe_sched_funcs {
  /* Function ultimately responsible for granting hart requests from a child
   * scheduler. This function is automatically called when a child invokes
   * lithe_hart_request() from within it's current scheduler. Returns 0 on
   * success, -1 on failure. */
  int (*hart_request) (lithe_sched_t *__this, lithe_sched_t *child, size_t k);

  /* Entry point for hart granted to this scheduler by a call to
   * lithe_hart_request(). */
  void (*hart_enter) (lithe_sched_t *__this);

  /* Entry point for harts given back to this scheduler by a call to
   * lithe_hart_yield(). */
  void (*hart_return) (lithe_sched_t *__this, lithe_sched_t *child);

  /* Callback to inform that a child scheduler has entered on one of the
   * current scheduler's harts */
  void (*child_enter) (lithe_sched_t *__this, lithe_sched_t *child);

  /* Callback to inform that a child scheduler has exited on one of the
   * current scheduler's harts */
  void (*child_exit) (lithe_sched_t *__this, lithe_sched_t *child);

  /* Callback letting this scheduler know that the provided context has been
   * blocked by some external component.  It will inform the scheduler when it
   * unblocks it via a call to lithe_context_unblock() which ultimately
   * triggers the context_unblock() callback. */
  void (*context_block) (lithe_sched_t *__this, lithe_context_t *context);

  /* Callback letting this scheduler know that the provided context has been
   * unblocked by some external component and is now resumable. */
  void (*context_unblock) (lithe_sched_t *__this, lithe_context_t *context);

  /* Callback notifying a scheduler that a context has cooperatively yielded
   * itself back to the scheduler. */
  void (*context_yield) (lithe_sched_t *__this, lithe_context_t *context);

  /* Callback notifying a scheduler that a context has run past the end of it's
   * start function and completed it's work.  At this point it should either be
   * reinitialized via a call to lithe_context_reinit() or cleaned up via
   * lithe_context_cleanup(). */
  void (*context_exit) (lithe_sched_t *__this, lithe_context_t *context);

} lithe_sched_funcs_t;

/* Basic lithe scheduler structure. All derived schedulers MUST have this as
 * their first field so that they can be cast properly within lithe */
struct lithe_sched {
  /* Scheduler functions. Must be set by the implementor of the second level
   * scheduler before calling lithe_sched_enter() */
  const lithe_sched_funcs_t *funcs;

  /* Main context. An UNITIALIZED lithe context sized for the scheduler specific
   * lithe context associated with the new scheduler. Must be set by the
   * implementor of the second level scheduler before calling lithe_sched_enter()
   * */
  lithe_context_t *main_context;

  /* All fields below are internal state managed internally by lithe. Put here
   * so that schedulers can statically create these objects if they wish to,
   * for performance reasons.  Alternate solution (previously used in older
   * versions of lithe) is to malloc space for this internal data when a new
   * scheduler is passed in, but mallocing and freeing on every
   * sched_enter/exit() proves costly.  Another solution would be to keep a
   * pool of scheduler data structures internal to lithe, but this adds
   * (arguably) unnecessary complexity. */

  /* Parent context when lithe_sched_enter() called */
  lithe_context_t *parent_context;

  /* Number of harts currently owned by this scheduler. */
  atomic_t harts;

  /* Scheduler's parent scheduler */
  lithe_sched_t *parent;
};

#ifdef __cplusplus
}
#endif

#endif  // LITHE_SCHED_H
