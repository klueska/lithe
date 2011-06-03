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
#include <vcore.h>
#include "lithe_test.h"


void enter(void *counter)
{
  fetch_and_add((int *) counter, 1);
  printf("Finished add: %d", vcore_id());
  lithe_sched_yield();
}


lithe_sched_funcs_t funcs = {
  .enter = enter,
  .yield = yield_nyi,
  .reg = reg_nyi,
  .unreg = unreg_nyi,
  .request = request_nyi,
  .unblock = unblock_nyi
};


void start(void *counter)
{
  int limit = ht_limit_hard_threads();
  lithe_sched_request(limit - 1);
  while (coherent_read(counter) < (limit - 1));
  lithe_sched_unregister();
}


int main()
{
  int counter = 0;
  lithe_sched_register(&funcs, (void *) &counter, start, (void *) &counter);
  return EXIT_SUCCESS;
}
