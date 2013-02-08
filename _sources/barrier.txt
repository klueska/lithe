Lithe Barriers
=================

To access the Lithe barrier API, include the following header file:
::

  #include <lithe/barrier.h>

Types
------------
::

  struct lithe_barrier;
  typedef struct lithe_barrier lithe_barrier_t;

.. c:type:: struct lithe_barrier
            lithe_barrier_t

API Calls
------------
::

  void lithe_barrier_init(lithe_barrier_t *barrier, int nthreads);
  void lithe_barrier_destroy(lithe_barrier_t *barrier);
  void lithe_barrier_wait(lithe_barrier_t *barrier);

.. c:function:: void lithe_barrier_init(lithe_barrier_t *barrier, int nthreads)
.. c:function:: void lithe_barrier_destroy(lithe_barrier_t *barrier)
.. c:function:: void lithe_barrier_wait(lithe_barrier_t *barrier)
