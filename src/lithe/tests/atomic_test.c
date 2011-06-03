#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <ht/atomic.h>

/* TODO: test mfence, cmpswp, fetchstore */

const int NTHREADS = 10;
const int NADDS = 1000;
const int ADDEND = 3;

void * fetchadd_test(void *ptr)
{
  int i;
  int fetched = 0;

  for (i = 0; i < (NADDS / NTHREADS); i++) {
    fetched += fetch_and_add((int *)ptr, ADDEND);
  }

  return (void *) (long) fetched;
}

int main(int argc, char **argv)
{
  int i;
  pthread_t pthds[NTHREADS];
  int added = 0; 
  int fetched = 0;

  for (i = 0; i < NTHREADS; i++) {
    pthread_create(&pthds[i], NULL, fetchadd_test, (void *) &added);
  }

  for (i = 0; i < NTHREADS; i++) {
    long int retval;
    pthread_join(pthds[i], (void *) &retval);
    fetched += retval;
  }

  assert(added == (NADDS * ADDEND));
  assert(fetched == (NADDS * (NADDS - 1) * ADDEND / 2));

  return EXIT_SUCCESS;
}

  

