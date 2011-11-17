/*
 * Simple Test:
 * 
 *   Registers a root scheduler which requests the maximum number 
 *   of harts, then verifies that the requested number of
 *   enters actually occur.
 *
 * Interface Tested: 
 *
 *   lithe_register  lithe_unregister  lithe_request  
 *   lithe_enter     lithe_yield       
*/

#include <stdlib.h>
#include <assert.h>

#include <parlib/parlib.h>
#include <lithe/lithe.h>
#include <lithe/defaults.h>

typedef struct test_sched {
  lithe_sched_t sched;
  unsigned int counter;
} test_sched_t;

static void vcore_enter(lithe_sched_t *__this);

static const lithe_sched_funcs_t funcs = {
  .vcore_request         = __vcore_request_default,
  .vcore_enter           = vcore_enter,
  .vcore_return          = __vcore_return_default,
  .child_entered         = __child_entered_default,
  .child_exited          = __child_exited_default,
  .context_block         = __context_block_default,
  .context_unblock       = __context_unblock_default,
  .context_yield         = __context_yield_default,
  .context_exit          = __context_exit_default
};

static void test_sched_ctor(test_sched_t *sched)
{
  sched->sched.funcs = &funcs;
  sched->counter = 0;
}

static void test_sched_dtor(test_sched_t *sched)
{
}

static void vcore_enter(lithe_sched_t *__this)
{
  unsigned int *counter = &((test_sched_t*)__this)->counter;
  printf("enter() on vcore %d\n", vcore_id());
  __sync_fetch_and_add(counter, 1);
  printf("counter: %d\n", *counter);
  lithe_vcore_yield();
}

static void test_run()
{
  printf("Scheduler Started!\n");
  test_sched_t *sched = (test_sched_t*)lithe_sched_current();
  for(int i=0; i<100; i++) {
    int limit, cur;
    do {
      limit = limit_vcores();
      cur = num_vcores();
    } while(!(limit - cur));
    sched->counter = 0;
    printf("counter: %d\n", sched->counter);
    printf("limit_vcores: %d\n", limit);
    printf("num_vcores: %d\n", cur);
    printf("Requesting vcores\n");
    lithe_vcore_request(limit - cur);
    printf("Waiting for counter to reach: %d\n", (limit - cur));
    while(sched->counter < (limit - cur))
      cpu_relax();
    printf("All vcores returned\n");
  }
  printf("Scheduler finishing!\n");
}

int main()
{
  printf("Lithe Simple test starting!\n");
  test_sched_t test_sched;
  test_sched_ctor(&test_sched);
  lithe_sched_enter((lithe_sched_t*)&test_sched);
  test_run();
  lithe_sched_exit();
  test_sched_dtor(&test_sched);
  printf("Lithe Simple test exiting\n");
  return 0;
}
