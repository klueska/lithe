Lithe Futexes
================

To access the Lithe futex API, include the following header file:
::

  #include <lithe/futex.h>

Constants
------------
::

  #define FUTEX_WAIT
  #define FUTEX_WAKE

.. c:macro:: FUTEX_WAIT
.. c:macro:: FUTEX_WAKE

API Calls
------------
::

  int futex(int *uaddr, int op, int val, const struct timespec *timeout,
            int *uaddr2, int val3);

.. c:function:: int futex(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3)

