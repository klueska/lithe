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
  /* Stack size for the lithe task */
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

  /* Size of the task's stack */
  size_t stack_size;

  /* Pointer to the task's stack */
  void *sp;

  /* Task local storage */
  void *tls;

  /* Flag indicating if the task is finished and should be destroyed or not */
  bool finished;

} lithe_task_t;

#ifdef __cplusplus
}
#endif

#endif  // LITHE_TASK_H
