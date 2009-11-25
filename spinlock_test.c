#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atomic.h"
#include "ht.h"
#include "spinlock.h"

static int lock = UNLOCKED;

static int i = 0;

static int done = 0;

void entry()
{
  spinlock_lock(&lock);
  {
    if (i < strlen("hello world\n")) {
      int j = i;
      for (; j < 100000000; j++);
      printf("%c", "hello world\n"[i++]);
      fflush(stdout);
    } else {
      done = 1;
    }
  }
  spinlock_unlock(&lock);
  ht_yield();
}


int main(int argc, char **argv)
{
  ht_request_async(15);
  while (coherent_read(done) == 0);
  return 0;
}
