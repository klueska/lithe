/*
 * Lithe Tasks
 */

#ifndef LITHE_TASK_H
#define LITHE_TASK_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <stdarg.h>
#include <uthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /* Stack bottom for a lithe task. */
  void *bottom;

  /* Stack size for a lithe task. */
  ssize_t size;

} lithe_task_stack_t;

typedef struct {
  /* The stack to initialize a lithe task to. If both stack_bot and stack_size
   * are set, initialize the lithe task with this stack. If stack_bot == NULL,
   * allocate a stack of size stack_size.  If stack_size == 0, allocate some
   * default stack size. */
  lithe_task_stack_t stack;

} lithe_task_attr_t;
  
/* Basic lithe task structure.  All derived scheduler tasks MUST have this as
 * their first field so that they can be cast properly within the lithe
 * scheduler. */
typedef struct lithe_task {
  /* Userlevel thread context. */
  uthread_t uth;

  /* Start function for the task */
  void (*start_func) (void *);

  /* Argument for the start function */
  void *arg;

  /* The task_stack associated with this task */
  lithe_task_stack_t stack;

  /* Task local storage */
  void *tls;

  /* Flag indicating if the task is finished and should be destroyed or not */
  bool finished;

  /* Flag indicating if the stack was provided or we need to ask the scheduler
   * to destroy it */
  bool free_stack;

} lithe_task_t;

#ifdef __cplusplus
}
#endif

#endif  // LITHE_TASK_H
