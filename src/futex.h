#ifndef LITHE_FUTEX_H
#define LITHE__FUTEX_H

#include <sys/time.h>

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1

#ifdef __cplusplus
extern "C" {
#endif

int futex(int *uaddr, int op, int val, const struct timespec *timeout,
          int *uaddr2, int val3);

#ifdef __cplusplus
}
#endif

#endif	/* _FUTEX_H */
