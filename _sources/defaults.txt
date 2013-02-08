Lithe Default Scheduler Callbacks
==================================

To access the Lithe default scheduler callbacks API, include the following
header file:
::

  #include <lithe/defaults.h>

API Calls
---------------
::

  lithe_context_t* __lithe_context_create_default(bool stack);
  void __lithe_context_destroy_default(lithe_context_t *context, bool stack);

  int __hart_request_default(lithe_sched_t *__this, lithe_sched_t *child, int k);
  void __hart_entry_default(lithe_sched_t *__this);
  void __hart_return_default(lithe_sched_t *__this, lithe_sched_t *child);
  void __child_enter_default(lithe_sched_t *__this, lithe_sched_t *child);
  void __child_exit_default(lithe_sched_t *__this, lithe_sched_t *child);
  void __context_block_default(lithe_sched_t *__this, lithe_context_t *context);
  void __context_unblock_default(lithe_sched_t *__this, lithe_context_t *context);
  void __context_yield_default(lithe_sched_t *__this, lithe_context_t *context);
  void __context_exit_default(lithe_sched_t *__this, lithe_context_t *context);

.. c:function:: lithe_context_t* __lithe_context_create_default(bool stack)



.. c:function:: void __lithe_context_destroy_default(lithe_context_t *context, bool stack)



.. c:function:: int __hart_request_default(lithe_sched_t *__this, lithe_sched_t *child, int k)



.. c:function:: void __hart_entry_default(lithe_sched_t *__this)



.. c:function:: void __hart_return_default(lithe_sched_t *__this, lithe_sched_t *child)



.. c:function:: void __child_enter_default(lithe_sched_t *__this, lithe_sched_t *child)



.. c:function:: void __child_exit_default(lithe_sched_t *__this, lithe_sched_t *child)



.. c:function:: void __context_block_default(lithe_sched_t *__this, lithe_context_t *context)



.. c:function:: void __context_unblock_default(lithe_sched_t *__this, lithe_context_t *context)



.. c:function:: void __context_yield_default(lithe_sched_t *__this, lithe_context_t *context)



.. c:function:: void __context_exit_default(lithe_sched_t *__this, lithe_context_t *context)



