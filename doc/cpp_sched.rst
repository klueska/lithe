Lithe C++ Schedulers
=======================

To access the Lithe C++ Scheduelers API, include the following header file:
::

  #include <lithe/sched.hh>

Namespaces
------------
::

  namespace lithe;

.. cpp:namespace:: lithe

Classes
---------
::

  class lithe::Scheduler : public lithe_sched_t {
   protected:
    virtual void hart_enter() = 0;
  
    virtual int hart_request(lithe_sched_t *child, int k);
    virtual void hart_return(lithe_sched_t *child);
    virtual void child_enter(lithe_sched_t *child);
    virtual void child_exit(lithe_sched_t *child);
    virtual void context_block(lithe_context_t *context);
    virtual void context_unblock(lithe_context_t *context);
    virtual void context_yield(lithe_context_t *context);
    virtual void context_exit(lithe_context_t *context);
  
   public:
    Scheduler();
    virtual ~Scheduler();
  };
  
.. cpp:class:: Scheduler : public lithe_sched_t

  :c:type:`lithe_sched_t`


.. cpp:function:: void Scheduler::hart_enter()



.. cpp:function:: int Scheduler::hart_request(lithe_sched_t *child, int k)



.. cpp:function:: void Scheduler::hart_return(lithe_sched_t *child)



.. cpp:function:: void Scheduler::child_enter(lithe_sched_t *child)



.. cpp:function:: void Scheduler::child_exit(lithe_sched_t *child)



.. cpp:function:: void Scheduler::context_block(lithe_context_t *context)



.. cpp:function:: void Scheduler::context_unblock(lithe_context_t *context)



.. cpp:function:: void Scheduler::context_yield(lithe_context_t *context)



.. cpp:function:: void Scheduler::context_exit(lithe_context_t *context)



.. cpp:function:: Scheduler::Scheduler()



.. cpp:function:: Scheduler::~Scheduler()



