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
#include <lithe/defaults.h>
#include <ht/atomic.h>

typedef struct test_sched {
  lithe_sched_t sched;
  unsigned int counter;
} test_sched_t;

static void test_sched_ctor(test_sched_t *sched)
{
  sched->counter = 0;
}

static void test_sched_dtor(test_sched_t *sched)
{
}

static void vcore_enter(lithe_sched_t *__this);

static const lithe_sched_funcs_t funcs = {
  .vcore_request      = __vcore_request_default,
  .vcore_enter        = vcore_enter,
  .vcore_return       = __vcore_return_default,
  .child_entered      = __child_entered_default,
  .child_exited       = __child_exited_default,
  .task_create        = __task_create_default,
  .task_destroy       = __task_destroy_default,
  .task_stack_create  = __task_stack_create_default,
  .task_stack_destroy = __task_stack_destroy_default,
  .task_runnable      = __task_runnable_default,
  .task_yield         = __task_yield_default
};

static void vcore_enter(lithe_sched_t *__this)
{
  unsigned int *counter = &((test_sched_t*)__this)->counter;
  printf("enter() on vcore %d\n", vcore_id());
  fetch_and_add(counter, 1);
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
      limit = max_vcores();
      cur = num_vcores();
    } while(!(limit - cur));
    sched->counter = 0;
    printf("counter: %d\n", sched->counter);
    printf("max_vcores: %d\n", limit);
    printf("num_vcores: %d\n", cur);
    printf("Requesting vcores\n");
    lithe_vcore_request(limit - cur);
    printf("Waiting for counter to reach: %d\n", (limit - cur));
    while (coherent_read(sched->counter) < (limit - cur));
    printf("All vcores returned\n");
  }
  printf("Scheduler finishing!\n");
}

int main()
{
  printf("Lithe Simple test starting!\n");
  test_sched_t test_sched;
  test_sched_ctor(&test_sched);
  lithe_sched_enter(&funcs, (lithe_sched_t*)&test_sched);
  test_run();
  lithe_sched_exit();
  test_sched_dtor(&test_sched);
  printf("Lithe Simple test exiting\n");
  return 0;
}
