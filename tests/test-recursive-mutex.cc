#include <src/mutex.h>

int internal_mutex_init(lithe_mutex_t *mutex, lithe_mutexattr_t *attr)
{
  lithe_mutexattr_t new_attr;
  lithe_mutexattr_init(&new_attr);
  lithe_mutexattr_settype(&new_attr, LITHE_MUTEX_RECURSIVE);
  return lithe_mutex_init(mutex, &new_attr);
}
#undef lithe_mutex_init
#define lithe_mutex_init internal_mutex_init

int internal_mutex_lock(lithe_mutex_t *mutex)
{
  int retval;
  for(int i=0; i<100; i++) {
    retval = lithe_mutex_lock(mutex);
    if(retval != 0)
      break;
  }
  return retval;
}
#undef lithe_mutex_lock
#define lithe_mutex_lock internal_mutex_lock

int internal_mutex_unlock(lithe_mutex_t *mutex)
{
  int retval;
  for(int i=0; i<100; i++) {
    retval = lithe_mutex_unlock(mutex);
    if(retval != 0)
      break;
  }
  return retval;
}
#undef lithe_mutex_unlock
#define lithe_mutex_unlock internal_mutex_unlock

#include "test-mutex.cc"
