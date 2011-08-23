/*
 * Lithe Contexts
 */

#ifndef LITHE_CONTEXT_H
#define LITHE_CONTEXT_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <stdarg.h>
#include <uthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  /* Stack bottom for a lithe context. */
  void *bottom;

  /* Stack size for a lithe context. */
  ssize_t size;

} lithe_context_stack_t;

typedef struct {
  /* The stack to initialize a lithe context to. If both stack_bot and stack_size
   * are set, initialize the lithe context with this stack. If stack_bot == NULL,
   * allocate a stack of size stack_size.  If stack_size == 0, allocate some
   * default stack size. */
  lithe_context_stack_t stack;

} lithe_context_attr_t;
  
/* Basic lithe context structure.  All derived scheduler contexts MUST have this as
 * their first field so that they can be cast properly within the lithe
 * scheduler. */
typedef struct lithe_context {
  /* Userlevel thread context. */
  uthread_t uth;

  /* Start function for the context */
  void (*start_func) (void *);

  /* Argument for the start function */
  void *arg;

  /* The context_stack associated with this context */
  lithe_context_stack_t stack;

  /* Context local storage */
  void *tls;

  /* Flag indicating if the context is finished and should be destroyed or not */
  bool finished;

  /* Flag indicating if the stack was provided or we need to ask the scheduler
   * to destroy it */
  bool free_stack;

} lithe_context_t;

#ifdef __cplusplus
}
#endif

#endif  // LITHE_CONTEXT_H
