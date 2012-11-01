/*
 * Lithe Contexts
 */

#ifndef LITHE_CONTEXT_H
#define LITHE_CONTEXT_H

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <stdarg.h>
#include <parlib/uthread.h>
#include "deque.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Declare types and functions for maintaining a queue of lithe contexts */
struct lithe_context;
typedef struct lithe_context lithe_context_t;
DECLARE_TYPED_DEQUE(lithe_context, lithe_context_t *);

enum {
  CONTEXT_READY,
  CONTEXT_BLOCKED,
  CONTEXT_YIELDED,
  CONTEXT_FINISHED,
};

/* Struct to maintain lithe stacks */
typedef struct {
  /* Stack bottom for a lithe context. */
  void *bottom;

  /* Stack size for a lithe context. */
  ssize_t size;

} lithe_context_stack_t;
  
/* Keys used to dynamically manage context local storage regions */
/* NOTE: The current implementation uses a locked linked list to find the key 
 * for a given thread. We should probably find a better way to do this based on
 * a custom lock-free hash table or something. */
#include <parlib/queue.h>
#include <parlib/spinlock.h>

/* The key structure itself */
typedef struct lithe_clskey {
  spinlock_t lock;
  int ref_count;
  bool valid;
  void (*dtor)(void*);
} lithe_clskey_t;

/* The definition of a clskey element */
typedef struct lithe_cls_list_element {
  TAILQ_ENTRY(lithe_cls_list_element) link;
  lithe_clskey_t *key;
  void *cls;
} lithe_cls_list_element_t;
TAILQ_HEAD(lithe_cls_list, lithe_cls_list_element);
typedef struct lithe_cls_list lithe_cls_list_t;
typedef void (*lithe_cls_dtor_t)(void*);

struct lithe_sched;
/* Basic lithe context structure.  All derived scheduler contexts MUST have this as
 * their first field so that they can be cast properly within the lithe
 * scheduler. */
struct lithe_context {
  /* Userlevel thread context. */
  uthread_t uth;

  /* The scheduler managing this context */
  struct lithe_sched *sched;

  /* Start function for the context */
  void (*start_func) (void *);

  /* Argument for the start function */
  void *arg;

  /* The context_stack associated with this context */
  lithe_context_stack_t stack;

  /* The list of cls chunks associated with this context */
  lithe_cls_list_t cls_list;

  /* State used internally by the lithe runtime to manage contexts */
  size_t state;

};

#ifdef __cplusplus
}
#endif

#endif  // LITHE_CONTEXT_H
