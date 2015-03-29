#include <stdio.h>
#include <unistd.h>

#include <parlib/parlib.h>
#include <parlib/spinlock.h>
#include <src/lithe.h>
#include <src/defaults.h>

typedef struct test_sched {
  lithe_sched_t sched;
} test_sched_t;

static void hart_enter(lithe_sched_t *__this);

static const lithe_sched_funcs_t funcs = {
  .hart_request        = __hart_request_default,
  .hart_enter          = hart_enter,
  .hart_return         = __hart_return_default,
  .sched_enter         = __sched_enter_default,
  .sched_exit          = __sched_exit_default,
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
  sched->sched.main_context = malloc(sizeof(lithe_context_t));
}

static void test_sched_dtor(test_sched_t *sched)
{
  free(sched->sched.main_context);
}

static void hart_enter(lithe_sched_t *__this)
{
  printf("enter() on hart %d\n", hart_id());
  lithe_hart_request(-1);
  lithe_hart_yield();
}

static void test_run()
{
  printf("Scheduler Started!\n");
  size_t limit = max_harts();
  for(int i=0; i<100; i++) {
    printf("max_harts: %lu\n", limit);
    printf("num_harts: %lu\n", num_harts());
    printf("Requesting harts\n");
    lithe_hart_request(limit - 1);
    while (num_harts() > 1)
      cpu_relax();
    printf("Finished iteration %d\n", i);
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
