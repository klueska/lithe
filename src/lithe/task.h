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
  /* Stack top for the lithe task. If stack_bot == NULL, then the lithe task
  * should allocate a new stack of size stack_size.  If stack_bot != NULL, then
  * the task should use this stack rather than allocating a new one; stack_size
  * must be set to the size of this stack. */
  void *stack_bot;

  /* Stack size for the lithe task. if stack_bot == NULL, stack_size indicates
   * the size of the stack the lithe_task should allocate; if it is 0, then
   * allocate some default size.  If stack_bot != NULL, this must contain the
   * size of the stack pointed to by stack_bot; if it is 0 there is an error.
   * */
  ssize_t stack_size;

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

  /* Pointer to the task's stack */
  void *stack_bot;

  /* Size of the task's stack */
  size_t stack_size;

  /* Task local storage */
  void *tls;

  /* Flag indicating if the task is finished and should be destroyed or not */
  bool finished;

  /* Flag indicating if the stack should be automatically freed or not */
  bool free_stack;

} lithe_task_t;

#ifdef __cplusplus
}
#endif

#endif  // LITHE_TASK_H
