#ifndef LITHE_FUTEX_H
#define LITHE__FUTEX_H

#include <sys/time.h>

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

#ifdef __cplusplus
extern "C" {
#endif

/* Traditional futex call, passing flags. */
int futex(int *uaddr, int op, int val, const struct timespec *timeout,
          int *uaddr2, int val3);

/* Simple wait call, that almost everyone usually makes.  Equivalent to 
 * futex(uaddr, FUTEX_WAIT, val, NULL, NULL, 0) */
int futex_wait(int *uaddr, int val);


/* Variants on the wakeup call, optimized for the most common cases of wake
 * one or wake all.*/
int futex_wake_one(int *uaddr);
int futex_wake_all(int *uaddr);
int futex_wake_some(int *uaddr, int count);

#define futex_wake(uaddr, count) \
( \
  __builtin_constant_p(count) && (count) == 1 ? futex_wake_one(uaddr) : \
  __builtin_constant_p(count) && (count) == INT_MAX ? futex_wake_all(uaddr) : \
  futex_wake_some(uaddr, count) \
)

#ifdef __cplusplus
}
#endif

#endif	/* _FUTEX_H */
