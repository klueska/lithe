Lithe Contexts
==================================

To access the Lithe contexts API, include the following header file:
::

  #include <lithe/context.h>

Types
----------------------------------
::

  struct lithe_context_stack;
  typedef lithe_context_stack lithe_context_stack_t;

  struct lithe_context;
  typedef struct lithe_context lithe_context_t;

  DECLARE_TYPED_DEQUE(lithe_context, lithe_context_t *);

.. c:type:: struct lithe_context_stack
            lithe_context_stack_t

  Struct to maintain lithe context stacks
  ::

    struct lithe_context_stack {
      void *bottom;
      ssize_t size;
    };

  .. c:member:: lithe_context_stack_t.bottom

    Stack bottom for a lithe context.

  .. c:member:: lithe_context_stack_t.size

    Stack size for a lithe context.
  
.. c:type:: struct lithe_context
            lithe_context_t

  Basic lithe context structure.  All derived scheduler contexts MUST have this
  as their first field so that they can be cast properly within the lithe
  scheduler.
  ::

    struct lithe_context {
      lithe_context_stack_t stack;
      ... // Other "private" fields
    };

  .. c:member:: lithe_context_t.stack
  
    The stack associated with this lithe context.  This is the only "public"
    field exposed through the lithe_context_t_ API. It should be set before calling
    :c:func:`lithe_context_init` on the context.

.. c:type:: struct lithe_context_deque

  A lithe context "deque" type so that lithe contexts can be enqueued and
  dequeued into list by components external to the lithe runtime. For example,
  the supplied lithe mutex, barrier and condvar implementations use this
  construct to hold blocked contexts and resume them later.

