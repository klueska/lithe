Lithe Contexts
==================================

Lithe Context Stacks
----------------------------------
::

  typedef struct {
    void *bottom;
    ssize_t size;
  } lithe_context_stack_t;

.. c:type:: lithe_context_stack_t

  A generic type representing the stack used by a lithe context.

.. c:member:: void *lithe_context_stack_t.bottom
  
  Pointer to the bottom of the stack for a lithe context.

.. c:member:: ssize_t lithe_context_stack_t.size
  
  Size of the stack pointed to by bottom.
  
Lithe Contexts
----------------------------------
::

  struct lithe_context {
    uthread_t uth;
    struct lithe_sched *sched;
    void (*start_func) (void *);
    void *arg;
    lithe_context_stack_t stack;
    void *cls;
    size_t state;
  };
  typedef struct lithe_context;

.. c:type:: struct lithe_context
            lithe_context_t

  Basic lithe context structure.  All derived scheduler contexts MUST have this
  as their first field so that they can be cast properly within the lithe
  scheduler.

.. c:member:: uthread_t lithe_context_t.uth
  
  Userlevel thread context.

.. c:member:: struct lithe_sched *lithe_context_t.sched

  The scheduler managing this context.

.. c:member:: void (*lithe_context_t.start_func) (void *)
  
  Start function for the context.

.. c:member:: void *lithe_context_t.arg
  
  Argument for the start function.

.. c:member:: lithe_context_stack_t lithe_context_t.stack
  
  The context_stack associated with this context.

.. c:member:: void *lithe_context_t.cls
  
  Context local storage.

.. c:member:: size_t lithe_context_t.state
  
  State used internally by the lithe runtime to manage contexts.

