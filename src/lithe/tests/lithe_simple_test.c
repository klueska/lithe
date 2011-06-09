/*
 * Simple Test:
 * 
 *   Registers a root scheduler which requests the maximum number 
 *   of hard threads, then verifies that the requested number of
 *   enters actually occur.
 *
 * Interface Tested: 
 *
 *   lithe_register  lithe_unregister  lithe_request  
 *   lithe_enter     lithe_yield       
*/

#include <stdlib.h>
#include <assert.h>

#include <lithe/lithe.h>
#include <ht/atomic.h>
#include "lithe_test.h"

void enter(void *counter);

lithe_sched_funcs_t funcs = {
  .enter = enter,
  .yield = yield_nyi,
  .reg = reg_nyi,
  .unreg = unreg_nyi,
  .request = request_nyi,
  .unblock = unblock_nyi
};

void yield()
{
}

void enter(void *__counter)
{
  unsigned int *counter = (unsigned int*)__counter;
  printf("enter() on vcore %d\n", vcore_id());
  fetch_and_add(counter, 1);
  printf("counter: %d\n", *counter);
  lithe_sched_yield();
}

void start(void *__counter)
{
  printf("Scheduler Started!\n");
  unsigned int *counter = (unsigned int*)__counter;
  for(int i=0; i<100; i++) {
    int limit = max_vcores();
    int cur = num_vcores();
    *counter = 0;
    printf("max_vcores: %d\n", limit);
    printf("num_vcores: %d\n", cur);
    printf("Requesting vcores\n");
    lithe_sched_request(limit - cur);
    while (coherent_read(*counter) < (limit - cur));
    printf("All vcores returned\n");
  }
  printf("Deregistering scheduler\n");
  lithe_sched_unregister();
}

int main()
{
  int counter = 0;
  printf("Lithe Simple test starting!\n");
  printf("Registering scheduler\n");
  lithe_sched_register(&funcs, (void *) &counter, start, (void *) &counter);
  printf("Lithe Simple test exiting\n");
  return EXIT_SUCCESS;
}
