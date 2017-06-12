Lithe C Schedulers
==================================

To access the Lithe C Scheduelers API, include the following header file:
::

  #include <lithe/sched.h>

Types
-----------
::

  struct lithe_sched;
  typedef struct lithe_sched lithe_sched_t;

  struct lithe_sched_funcs;
  typedef struct struct lithe_sched_funcs lithe_sched_funcs_t;

.. c:type:: struct lithe_sched
.. c:type:: lithe_sched_t

  Opaque scheduler type passed around between the lithe runtime and any
  user-level schedulers built on top of lithe.

.. c:type:: struct lithe_sched_funcs
.. c:type:: lithe_sched_funcs_t

  A struct containing all of the callback functions that any user-level
  scheduler built on top of lithe must implement.
  ::

    struct lithe_sched_funcs {
      int (*hart_request) (lithe_sched_t *__this, lithe_sched_t *child, int k);
      void (*hart_enter) (lithe_sched_t *__this);
      void (*hart_return) (lithe_sched_t *__this, lithe_sched_t *child);
      void (*child_enter) (lithe_sched_t *__this, lithe_sched_t *child);
      void (*child_exit) (lithe_sched_t *__this, lithe_sched_t *child);
      void (*context_block) (lithe_sched_t *__this, lithe_context_t *context);
      void (*context_unblock) (lithe_sched_t *__this, lithe_context_t *context);
      void (*context_yield) (lithe_sched_t *__this, lithe_context_t *context);
      void (*context_exit) (lithe_sched_t *__this, lithe_context_t *context);
    };

  .. c:function:: int lithe_sched_funcs_t.hart_request(lithe_sched_t *__this, lithe_sched_t *child, int k)
  
    Function ultimately responsible for granting hart requests from a child
    scheduler. This function is automatically called when a child invokes
    lithe_hart_request() from within it's current scheduler. Returns 0 on
    success, -1 on failure.
  
  .. c:function:: void lithe_sched_funcs_t.hart_enter(lithe_sched_t *__this)
  
    Entry point for hart granted to this scheduler by a call to
    lithe_hart_request().
  
  .. c:function:: void lithe_sched_funcs_t.hart_return(lithe_sched_t *__this, lithe_sched_t *child)
  
    Entry point for harts given back to this scheduler by a call to
    lithe_hart_yield().
  
  .. c:function:: void lithe_sched_funcs_t.child_enter(lithe_sched_t *__this, lithe_sched_t *child)
  
    Callback to inform that a child scheduler has entered on one of the
    current scheduler's harts.
  
  .. c:function:: void lithe_sched_funcs_t.child_exit(lithe_sched_t *__this, lithe_sched_t *child)
  
    Callback to inform that a child scheduler has exited on one of the
    current scheduler's harts.
  
  .. c:function:: void lithe_sched_funcs_t.context_block(lithe_sched_t *__this, lithe_context_t *context)
  
    Callback letting this scheduler know that the provided context has been
    blocked by some external component.  It will inform the scheduler when it
    unblocks it via a call to lithe_context_unblock() which ultimately
    triggers the context_unblock() callback.
  
  .. c:function:: void lithe_sched_funcs_t.context_unblock(lithe_sched_t *__this, lithe_context_t *context)
  
    Callback letting this scheduler know that the provided context has been
    unblocked by some external component and is now resumable.
  
  .. c:function:: void lithe_sched_funcs_t.context_yield(lithe_sched_t *__this, lithe_context_t *context)
  
    Callback notifying a scheduler that a context has cooperatively yielded
    itself back to the scheduler.
  
  .. c:function:: void lithe_sched_funcs_t.context_exit(lithe_sched_t *__this, lithe_context_t *context)
  
    Callback notifying a scheduler that a context has run past the end of its
    start function and completed its work.  At this point it should either be
    reinitialized via a call to lithe_context_reinit() (and friends) or cleaned
    up via lithe_context_cleanup().
  
