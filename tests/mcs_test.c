#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atomic.h"
#include "ht.h"
#include "mcs.h"

static struct qnode *lock = NULL;

static int i = 0;

static int done = 0;

void entry()
{
  struct qnode q;
  mcs_lock(&lock, &q);
  {
    if (i < strlen("hell\n")) {
      int j = i;
      for (; j < 100000000; j++);
      printf("%c", "hell\n"[i++]);
      fflush(stdout);
    } else {
      done = 1;
    }
  }
  mcs_unlock(&lock, &q);
  ht_yield();
}


int main(int argc, char **argv)
{
  ht_request_async(15);
  while (coherent_read(done) == 0);
  return 0;
}
