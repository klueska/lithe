#ifndef FUTEX_H
#define FUTEX_H

#include <errno.h>
#include <limits.h>
#include <unistd.h>

#ifndef __linux__
#error "expecting __linux__ (for now, hard threads only run on Linux)"
#endif

#include <linux/futex.h>
#include <sys/syscall.h>


inline void futex_wait(void *futex, int comparand)
{
  int r = syscall(SYS_futex, futex, FUTEX_WAIT, comparand, NULL, NULL, 0);
  if (!(r == 0 || r == EWOULDBLOCK ||
      (r == -1 && (errno == EAGAIN || errno == EINTR)))) {
    fprintf(stderr, "ht: futex_wait failed");
    exit(1);
  }
}


inline void futex_wakeup_one(void *futex)
{
  int r = syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, NULL, 0);
  if (!(r == 0 || r == 1)) {
    fprintf(stderr, "ht: futex_wakeup_on failed");
    exit(1);
  }
}


inline void futex_wakeup_all(void *futex)
{
  int r = syscall(SYS_futex, futex, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
  if (!(r >= 0)) {
    fprintf(stderr, "ht: futex_wakeup_all failed");
    exit(1);
  }
}


#endif /* FUTEX_H */
