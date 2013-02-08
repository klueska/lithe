Lithe Condition Variables
===========================

To access the Lithe condition variable API, include the following header file:
::

  #include <lithe/condvar.h>

Types
------------
::

  struct lithe_condvar;
  typedef struct lithe_condvar  lithe_condvar_t;

.. c:type:: struct lithe_condvar
            lithe_condvar_t

API Calls
------------
::

  int lithe_condvar_init(lithe_condvar_t* c);
  int lithe_condvar_wait(lithe_condvar_t* c, lithe_mutex_t* m);
  int lithe_condvar_signal(lithe_condvar_t* c);
  int lithe_condvar_broadcast(lithe_condvar_t* c);

.. c:function:: int lithe_condvar_init(lithe_condvar_t* c)

  Initialize a condition variable.

.. c:function:: int lithe_condvar_wait(lithe_condvar_t* c, lithe_mutex_t* m)

  Wait on a condition variable.

.. c:function:: int lithe_condvar_signal(lithe_condvar_t* c)

  Signal the next lithe context waiting on the condition variable.

.. c:function:: int lithe_condvar_broadcast(lithe_condvar_t* c)

  Broadcast a signal to all lithe contexts waiting on the condition variable.

