#include <stdio.h>
#include <unistd.h>

#include <parlib/parlib.h>
#include <src/lithe.h>
#include <src/defaults.h>

typedef struct test_sched {
  lithe_sched_t sched;
  unsigned int counter;
} test_sched_t;

static void hart_enter(lithe_sched_t *__this);

static const lithe_sched_funcs_t funcs = {
  .hart_request        = __hart_request_default,
  .hart_enter          = hart_enter,
  .hart_return         = __hart_return_default,
  .child_enter         = __child_enter_default,
  .child_exit          = __child_exit_default,
  .context_block       = __context_block_default,
  .context_unblock     = __context_unblock_default,
  .context_yield       = __context_yield_default,
  .context_exit        = __context_exit_default
};

static void test_sched_ctor(test_sched_t *sched)
{
  sched->sched.funcs = &funcs;
  sched->counter = 0;
}

static void test_sched_dtor(test_sched_t *sched)
{
}

static void hart_enter(lithe_sched_t *__this)
{
  unsigned int *counter = &((test_sched_t*)__this)->counter;
  printf("enter() on hart %d\n", hart_id());
  __sync_fetch_and_add(counter, 1);
  printf("counter: %d\n", *counter);
  lithe_hart_yield();
}

static void test_run()
{
  printf("Scheduler Started!\n");
  test_sched_t *sched = (test_sched_t*)lithe_sched_current();
  unsigned int limit = max_harts();
  if(limit == 1) {
    printf("ERROR: This simple test spins on one hart.\n");
    printf("       Therefore, it requires at least 2 harts in order to run.\n");
    printf("       Other tests in this suite are more sophisticated and should run just fine.\n");
    printf("       Are you running this test on a machine with only 1 CPU?\n");
    exit(1);
  }
  for(int i=0; i<100; i++) {
    unsigned int cur;
    do {
      cur = num_harts();
      cpu_relax();
    } while(!(limit - cur));
    sched->counter = 0;
    printf("counter: %d\n", sched->counter);
    printf("max_harts: %d\n", limit);
    printf("num_harts: %d\n", cur);
    printf("Requesting harts\n");
    lithe_hart_request(limit - cur);
    printf("Waiting for counter to reach: %d\n", (limit - cur));
    while(sched->counter < (limit - cur))
      cpu_relax();
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
