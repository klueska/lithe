#ifndef LITHE_FUTEX_H
#define LITHE__FUTEX_H

#include <sys/time.h>

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

int futex(int *uaddr, int op, int val, const struct timespec *timeout,
          int *uaddr2, int val3);

#endif	/* _FUTEX_H */
