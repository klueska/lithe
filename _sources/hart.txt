Harts
===================

To access the harts API, include the following header file:
::

  #include <lithe/hart.h>

API Calls
------------
::

  bool in_hart_context();
  int hart_id();
  size_t num_harts();
  size_t max_harts();
  #define hart_set_tls_var(name, val)
  #define hart_get_tls_var(name)

.. c:function:: bool in_hart_context()
.. c:function:: int hart_id()
.. c:function:: size_t num_harts()
.. c:function:: size_t max_harts()
.. c:function:: #define hart_set_tls_var(name, val)
.. c:function:: #define hart_get_tls_var(name)
