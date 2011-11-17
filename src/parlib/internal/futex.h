/*
 * Copyright (c) 2011 The Regents of the University of California
 * Benjamin Hindman <benh@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef FUTEX_H
#define FUTEX_H

#include <errno.h>
#include <limits.h>
#include <unistd.h>

#ifndef __linux__
#error "expecting __linux__ (for now, this library only runs on Linux)"
#endif

#include <linux/futex.h>
#include <sys/syscall.h>


inline static void futex_wait(void *futex, int comparand)
{
  int r = syscall(SYS_futex, futex, FUTEX_WAIT, comparand, NULL, NULL, 0);
  if (!(r == 0 || r == EWOULDBLOCK ||
      (r == -1 && (errno == EAGAIN || errno == EINTR)))) {
    fprintf(stderr, "futex: futex_wait failed");
    exit(1);
  }
}


inline static void futex_wakeup_one(void *futex)
{
  int r = syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, NULL, 0);
  if (!(r == 0 || r == 1)) {
    fprintf(stderr, "futex: futex_wakeup_on failed");
    exit(1);
  }
}


inline static void futex_wakeup_all(void *futex)
{
  int r = syscall(SYS_futex, futex, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);
  if (!(r >= 0)) {
    fprintf(stderr, "futex: futex_wakeup_all failed");
    exit(1);
  }
}


#endif /* FUTEX_H */
