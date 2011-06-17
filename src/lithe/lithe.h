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

/* Opaque type for lithe schedulers. */
typedef struct lithe_sched lithe_sched_t;

/* Struct to hold the stack pointer and size of a lithe task stack */
struct lithe_task_stack {
  /* The stack pointer */
  void *sp;

  /* The size of the stack */
  size_t size;
};
typedef struct lithe_task_stack lithe_task_stack_t;

/* Lithe task structure itself. */
struct lithe_task {
  /* Userlevel thread context. */
  uthread_t uth;

  /* The lithe scheduler associated with this task */
  lithe_sched_t *sched;

  /* Struct holding the stack pointer and its size for this task.  This is only
   * needed when a stack is created for this task dynamically, as indicataed by
   * the following field */
  lithe_task_stack_t stack;

  /* Flag indicating whether this task's stack was created dynamically, or one
   * was supplied for it at creation time */
  bool dynamic_stack;
};
typedef struct lithe_task lithe_task_t;

/* Lithe scheduler callbacks/entrypoints. */
struct lithe_sched_funcs {
  /* Entry point for vcores from the parent scheduler. */
  void (*enter) (void *__this);

  /* Entry point for vcores from a child scheduler. */
  void (*yield) (void *__this, lithe_sched_t *child);

  /* Callback (on the parent) to inform of a registerd child. */
  void (*reg) (void *__this, lithe_sched_t *child);

  /* Callback (on the parent) to inform of an unregistered child. */
  void (*unreg) (void *__this, lithe_sched_t *child);

  /* Callback (on the parent) to inform of child's request for vcores. */
  int (*request) (void *__this, lithe_sched_t *child, int k);

  /* Callback (on any scheduler) to inform of a resumable task. */
  void (*unblock) (void *__this, lithe_task_t *task);
};
typedef struct lithe_sched_funcs lithe_sched_funcs_t;
  

/**
 * Create a new scheduler instance (lithe_sched_t), registers it with
 * the current scheduler, and updates the vcore to be scheduled by this new
 * scheduler. Returns -1 if there is an error and sets errno appropriately.
 */
int lithe_sched_register(const lithe_sched_funcs_t *funcs,
			      void *__this,
			      lithe_task_t *task);

/**
 * Unregister the current scheduler. This call blocks the current vcore until
 * all resources have been recovered.
 * TODO: Add return semantics comment.
 */
int lithe_sched_unregister();

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
int lithe_sched_request(int k);

/**
 * Grant current vcore to specified scheduler. The specified
 * scheduler must be non-null and a registered scheduler of the
 * current scheduler. This function never returns unless child is
 * NULL, in which case it sets errno appropriately and returns -1.
 */
int lithe_sched_enter(lithe_sched_t *child);

/**
 * Reenter current scheduler. This function should never return.
 */
void lithe_sched_reenter();

/**
 * Yield current vcore to parent scheduler. This function should
 * never return.
 */
int lithe_sched_yield();

/*
 * Initialize a new task. Returns the newly initialized task on success and
 * NULL on error.
 */
lithe_task_t *lithe_task_create(void (*func) (void *), void *arg, 
                                lithe_task_stack_t *stack);

/*
 * Destroy an existing task. Returns 0 on success and -1 on error and
 * sets errno appropriately.
 */
int lithe_task_destroy(lithe_task_t *task);

/*
 * Returns the currently executing task.
 */
lithe_task_t *lithe_task_self();

/*
 * Run the specified task.  It must have been presreated. This 
 * function never returns on success and returns -1 on error and sets
 * errno appropriately.
 */
int lithe_task_run(lithe_task_t *task);

/**
 * Invoke the specified function with the current task. Note that you
 * can not block without a valid task (i.e. you can not block when
 * executing on the trampoline). Returns 0 on success (after the task
 * has been resumed) and -1 on error and sets errno appropriately.
*/
int lithe_task_block(void (*func) (lithe_task_t *, void *), void *arg);

/**
 * Notifies the current scheduler that the specified task is
 * resumable. Returns 0 on success and -1 on error and sets errno
 * appropriately.
 */
int lithe_task_unblock(lithe_task_t *task);

#ifdef __cplusplus
}
#endif

#endif  // LITHE_H
