Lithe Semaphores
===================

To access the Lithe semaphores API, include the following header file:
::

  #include <lithe/semaphore.h>

Constants
------------
::

  #define LITHE_SEM_INITIALIZER

.. c:macro:: LITHE_SEM_INITIALIZER

Types
------------
::

  struct lithe_sem;
  typedef struct lithe_sem lithe_sem_t;

.. c:type:: struct lithe_sem;
             lithe_sem_t;

API Calls
------------
::

  int lithe_sem_init(lithe_sem_t *sem, int count);
  int lithe_sem_wait(lithe_sem_t *sem);
  int lithe_sem_post(lithe_sem_t *sem);

.. c:function:: int lithe_sem_init(lithe_sem_t *sem, int count)

  Initialize a lithe semaphore.

.. c:function:: int lithe_sem_wait(lithe_sem_t *sem)

  Wait on a lithe semaphore.

.. c:function:: int lithe_sem_post(lithe_sem_t *sem)

  Post on a lithe semaphore.
