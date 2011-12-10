Lithe Schedulers
==================================
Callback API
----------------------------------
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
  typedef struct lithe_sched_funcs lithe_sched_funcs_t;

.. c:type:: struct lithe_sched_funcs
            lithe_sched_funcs_t

  Lithe scheduler callbacks/entrypoints.

.. c:member:: int (*lithe_sched_funcs_t.hart_request) (lithe_sched_t *__this, lithe_sched_t *child, int k)

  Function ultimately responsible for granting hart requests from a child
  scheduler. This function is automatically called when a child invokes
  lithe_hart_request() from within it's current scheduler. Returns 0 on
  success, -1 on failure.

.. c:member:: void (*lithe_sched_funcs_t.hart_enter) (lithe_sched_t *__this)

  Entry point for hart granted to this scheduler by a call to
  lithe_hart_request().

.. c:member:: void (*lithe_sched_funcs_t.hart_return) (lithe_sched_t *__this, lithe_sched_t *child)
  
  Entry point for harts given back to this scheduler by a call to
  lithe_hart_yield().

.. c:member:: void (*lithe_sched_funcs_t.child_enter) (lithe_sched_t *__this, lithe_sched_t *child)

  Callback to inform that a child scheduler has entered on one of the
  current scheduler's harts.

.. c:member:: void (*lithe_sched_funcs_t.child_exit) (lithe_sched_t *__this, lithe_sched_t *child)

  Callback to inform that a child scheduler has exited on one of the
  current scheduler's harts.

.. c:member:: void (*lithe_sched_funcs_t.context_block) (lithe_sched_t *__this, lithe_context_t *context)

  Callback letting this scheduler know that the provided context has been
  blocked by some external component.  It will inform the scheduler when it
  unblocks it via a call to lithe_context_unblock() which ultimately
  triggers the context_unblock() callback.

.. c:member:: void (*lithe_sched_funcs_t.context_unblock) (lithe_sched_t *__this, lithe_context_t *context)

  Callback letting this scheduler know that the provided context has been
  unblocked by some external component and is now resumable.

.. c:member:: void (*lithe_sched_funcs_t.context_yield) (lithe_sched_t *__this, lithe_context_t *context)

  Callback notifying a scheduler that a context has cooperatively yielded
  itself back to the scheduler.

.. c:member:: void (*lithe_sched_funcs_t.context_exit) (lithe_sched_t *__this, lithe_context_t *context)

  Callback notifying a scheduler that a context has run past the end of it's
  start function and completed it's work.  At this point it should either be
  reinitialized via a call to lithe_context_reinit() or cleaned up via
  lithe_context_cleanup().

Lithe Schedulers
----------------------------------
::

  struct lithe_sched {
    const lithe_sched_funcs_t *funcs;
    int harts;
    lithe_sched_t *parent;
    lithe_context_t *parent_context;
    lithe_context_t start_context;
  };
  typedef struct lithe_sched lithe_sched_t;

.. c:type:: struct lithe_sched
            lithe_sched_t

  Basic lithe scheduler structure. All derived schedulers MUST have this as
  their first field so that they can be cast properly within lithe.

.. c:member:: const lithe_sched_funcs_t *lithe_sched_t.funcs

  Scheduler functions. Must be set by the implementor of the second level
  scheduler before calling lithe_sched_entry().

.. c:member:: int lithe_sched_t.harts

  Number of harts currently owned by this scheduler.

.. c:member:: lithe_sched_t *lithe_sched_t.parent
  
  Scheduler's parent scheduler.

.. c:member:: lithe_context_t *lithe_sched_t.parent_context

  Parent context from which this scheduler was started.

.. c:member:: lithe_context_t lithe_sched_t.start_context
  
  The start context for this scheduler when it is first entered.

