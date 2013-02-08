Lithe Mutexes
================

To access the Lithe mutex API, include the following header file:
::

  #include <lithe/mutex.h>

Constants
------------
::

  enum {
    LITHE_MUTEX_NORMAL,
    LITHE_MUTEX_RECURSIVE,
    NUM_LITHE_MUTEX_TYPES,
  };
  #define LITHE_MUTEX_DEFAULT

  #define LITHE_MUTEX_INITIALIZER

.. c:macro:: LITHE_MUTEX_NORMAL
.. c:macro:: LITHE_MUTEX_RECURSIVE
.. c:macro:: NUM_LITHE_MUTEX_TYPES
.. c:macro:: LITHE_MUTEX_DEFAULT
.. c:macro:: LITHE_MUTEX_INITIALIZER

Types
------------
::

  struct lithe_mutxattr;
  typedef struct lithe_mutexattr lithe_mutexattr_t;

  struct lithe_mutex;
  typedef struct lithe_mutex lithe_mutex_t;

.. c:type:: struct lithe_mutexattr
            lithe_mutexattr_t


.. c:type:: struct lithe_mutex
            lithe_mutex_t

API Calls
------------
::

  int lithe_mutexattr_init(lithe_mutexattr_t *attr);
  int lithe_mutexattr_settype(lithe_mutexattr_t *attr, int type);
  int lithe_mutexattr_gettype(lithe_mutexattr_t *attr, int *type);

  int lithe_mutex_init(lithe_mutex_t *mutex, lithe_mutexattr_t *attr);
  int lithe_mutex_trylock(lithe_mutex_t *mutex);
  int lithe_mutex_lock(lithe_mutex_t *mutex);
  int lithe_mutex_unlock(lithe_mutex_t *mutex);

.. c:function:: int lithe_mutexattr_init(lithe_mutexattr_t *attr)

.. c:function:: int lithe_mutexattr_settype(lithe_mutexattr_t *attr, int type)

.. c:function:: int lithe_mutexattr_gettype(lithe_mutexattr_t *attr, int *type)

.. c:function:: int lithe_mutex_init(lithe_mutex_t *mutex, lithe_mutexattr_t *attr)

  Initialize a lithe mutex.

.. c:function:: int lithe_mutex_trylock(lithe_mutex_t *mutex)

  Try and lock a lithe mutex.

.. c:function:: int lithe_mutex_lock(lithe_mutex_t *mutex)

  Lock a lithe mutex.

.. c:function:: int lithe_mutex_unlock(lithe_mutex_t *mutex)

  Unlock a lithe mutex.

