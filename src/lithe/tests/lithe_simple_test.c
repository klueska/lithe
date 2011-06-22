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

typedef struct simple_sched {
  lithe_sched_t sched;
  unsigned int counter;
} simple_sched_t;

static lithe_sched_t *create();
static void destroy(lithe_sched_t *__this);
static void start(lithe_sched_t *__this);
static void vcore_enter(lithe_sched_t *__this);

static const lithe_sched_funcs_t funcs = {
  .create          = create,
  .destroy         = destroy,
  .start           = start,
  .vcore_request   = __vcore_request_default,
  .vcore_enter     = vcore_enter,
  .vcore_return    = __vcore_return_default,
  .child_started   = __child_started_default,
  .child_finished  = __child_finished_default,
  .task_create     = __task_create_default,
  .task_yield      = __task_yield_default,
  .task_exit       = __task_exit_default,
  .task_runnable   = __task_runnable_default
};

static lithe_sched_t *create()
{
  simple_sched_t *sched = malloc(sizeof(simple_sched_t));
  sched->counter = 0;
  return (lithe_sched_t*)sched;
}

static void destroy(lithe_sched_t *__this)
{
  assert(__this);
  free(__this);
}

static void vcore_enter(lithe_sched_t *__this)
{
  unsigned int *counter = &((simple_sched_t*)__this)->counter;
  printf("enter() on vcore %d\n", vcore_id());
  fetch_and_add(counter, 1);
  printf("counter: %d\n", *counter);
  lithe_vcore_yield();
}

static void start(lithe_sched_t *__this)
{
  printf("Scheduler Started!\n");
  unsigned int *counter = &((simple_sched_t*)__this)->counter;
  for(int i=0; i<100; i++) {
    int limit = max_vcores();
    int cur = num_vcores();
    *counter = 0;
    printf("counter: \n");
    printf("max_vcores: %d\n", limit);
    printf("num_vcores: %d\n", cur);
    printf("Requesting vcores\n");
    lithe_vcore_request(limit - cur);
    printf("Waiting for counter to reach: %d\n", (limit - cur));
    while (coherent_read(*counter) < (limit - cur));
    printf("All vcores returned\n");
  }
  printf("Scheduler finishing!\n");
}

int main()
{
  printf("Lithe Simple test starting!\n");
  lithe_sched_start(&funcs);
  printf("Lithe Simple test exiting\n");
  return EXIT_SUCCESS;
}
