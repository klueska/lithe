/*
 * Lithe Schedulers
 */

#ifndef LITHE_SCHED_H
#define LITHE_SCHED_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <lithe/context.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lithe sched type passed around between the lithe runtime and 2nd-level
 * schedulers */
struct lithe_sched;
typedef struct lithe_sched lithe_sched_t;

/* Lithe scheduler callbacks/entrypoints. */
typedef struct lithe_sched_funcs {
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

  /* Callback letting this scheduler know that the provided context has been
   * blocked by some external component.  It will inform the scheduler when it
   * unblocks it via the context_unblock() callback. */
  void (*context_block) (lithe_sched_t *__this, lithe_context_t *context);

  /* Callback letting this scheduler know that the provided context has been
   * unblocked. */
  void (*context_unblock) (lithe_sched_t *__this, lithe_context_t *context);

  /* Callback for scheduler specific yielding of contexts */
  void (*context_yield) (lithe_sched_t *__this, lithe_context_t *context);

  /* Callback for scheduler specific yielding of contexts */
  void (*context_exit) (lithe_sched_t *__this, lithe_context_t *context);

} lithe_sched_funcs_t;

/* Basic lithe scheduler structure. All derived schedulers MUST have this as
 * their first field so that they can be cast properly within lithe */
struct lithe_sched {
  /* Scheduler functions. Must be set by the implementor of the second level
   * scheduler before calling lithe_sched_entry() */
  const lithe_sched_funcs_t *funcs;

  /* All fields below are internal state managed internally by lithe. Put here
   * so that schedulers can statically create these objects if they wish to,
   * for performance reasons.  Alternate solution (previously used in older
   * versions of lithe) is to malloc space for this internal data when a new
   * scheduler is passed in, but mallocing and freeing on every
   * sched_entry/exit() proves costly.  Another solution would be to keep a
   * pool of scheduler data structures internal to lithe, but this adds
   * (arguably) unnecessary complexity. */

  /* Number of vcores currently owned by this scheduler. */
  int vcores;

  /* Scheduler's parent scheduler */
  lithe_sched_t *parent;

  /* Parent context from which this scheduler was started. */
  lithe_context_t *parent_context;

  /* The start context for this scheduler when it is first entered */
  lithe_context_t start_context;

};

#ifdef __cplusplus
}
#endif

#endif  // LITHE_SCHED_H
