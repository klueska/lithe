#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <parlib/parlib.h>
#include <parlib/mcs.h>
#include <sys/queue.h>
#include <src/lithe.h>
#include <src/fork_join_sched.h>
#include <src/defaults.h>

typedef struct child_sched {
  lithe_sched_t sched;
  lithe_context_t *start_context;
} child_sched_t;

static void child_hart_enter(lithe_sched_t *__this);
static void child_context_unblock(lithe_sched_t *__this, lithe_context_t *context); 

static const lithe_sched_funcs_t child_funcs = {
  .hart_request         = __hart_request_default,
  .hart_enter           = child_hart_enter,
  .hart_return          = __hart_return_default,
  .sched_enter          = __sched_enter_default,
  .sched_exit           = __sched_exit_default,
  .child_enter          = __child_enter_default,
  .child_exit           = __child_exit_default,
  .context_block         = __context_block_default,
  .context_unblock       = child_context_unblock,
  .context_yield         = __context_yield_default,
  .context_exit          = __context_exit_default,
};

static void child_sched_ctor(child_sched_t *sched)
{
  sched->sched.funcs = &child_funcs;
  sched->sched.main_context = malloc(sizeof(lithe_context_t));
  sched->start_context = NULL;
}

static void child_sched_dtor(child_sched_t *sched)
{
  free(sched->sched.main_context);
}

static void child_context_unblock(lithe_sched_t *__this, lithe_context_t *context) 
{
  printf("unblock_child\n");
  child_sched_t *sched = (child_sched_t *)__this;
  assert(context == sched->start_context);
  lithe_context_run(context);
}

void child_hart_enter(lithe_sched_t *__this) 
{
  child_sched_t *sched = (child_sched_t *)__this;
  lithe_context_t *context = sched->start_context;
  lithe_context_unblock(context);
}

static void block_child(lithe_context_t *context, void *arg)
{
  printf("block_child\n");
  child_sched_t *sched = (child_sched_t*)arg;
  sched->start_context = context;
}

static void child_main(void *arg)
{
  printf("child_main start\n");
  child_sched_t child_sched;
  child_sched_ctor(&child_sched);
  lithe_sched_enter((lithe_sched_t*)&child_sched);
  lithe_context_block(block_child, &child_sched);
  lithe_sched_exit();
  child_sched_dtor(&child_sched);
  printf("child_main finish\n");
}

static void middle_main(void *arg)
{
  printf("middle_main start\n");
  lithe_fork_join_sched_t *sched = lithe_fork_join_sched_create();
  lithe_sched_enter((lithe_sched_t*)sched);
  for (size_t i = 0; i < max_harts() - 1; i++)
    lithe_fork_join_context_create(sched, 262144, child_main, NULL);
  lithe_fork_join_sched_join_all(sched);
  lithe_sched_exit();
  lithe_fork_join_sched_destroy(sched);
  printf("middle_main finish\n");
}

static void root_main(void *arg)
{
  printf("root_main start\n");
  lithe_fork_join_sched_t *sched = lithe_fork_join_sched_create();
  lithe_sched_enter((lithe_sched_t*)sched);
  for (size_t i = 0; i < max_harts() - 1; i++)
    lithe_fork_join_context_create(sched, 262144, middle_main, NULL);
  lithe_fork_join_sched_join_all(sched);
  lithe_sched_exit();
  lithe_fork_join_sched_destroy(sched);
  printf("root_main finish\n");
}

int main()
{
  printf("main start\n");
  /* Start the root scheduler: Blocks until scheduler finishes */
  lithe_fork_join_sched_t *root_sched = lithe_fork_join_sched_create();
  lithe_sched_enter((lithe_sched_t*)root_sched);
  for (size_t i = 0; i < max_harts() - 1; i++)
    lithe_fork_join_context_create(root_sched, 262144, root_main, NULL);
  lithe_fork_join_sched_join_all(root_sched);
  lithe_sched_exit();
  lithe_fork_join_sched_destroy(root_sched);
  printf("main finish\n");
  return 0;
}
