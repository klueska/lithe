#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <parlib/parlib.h>
#include <parlib/mcs.h>
#include <sys/queue.h>
#include <src/lithe.hh>
#include <src/fork_join_sched.h>
#include <src/defaults.h>

using namespace lithe;

class ChildScheduler : public Scheduler {
 protected:
  void hart_enter();
  void context_unblock(lithe_context_t *context); 

 public:
  lithe_context_t *start_context;

  ChildScheduler();
  ~ChildScheduler();
};

ChildScheduler::ChildScheduler()
{
  this->main_context = new lithe_context_t();
  this->start_context = NULL;
}

ChildScheduler::~ChildScheduler()
{
  delete this->main_context;
}

void ChildScheduler::context_unblock(lithe_context_t *context) 
{
  printf("ChildScheduler::unblock_child\n");
  assert(context == this->start_context);
  lithe_context_run(context);
}

void ChildScheduler::hart_enter()
{
  printf("ChildScheduler::hart_enter\n");
  lithe_context_unblock(this->start_context);
}

static void block_child(lithe_context_t *context, void *arg)
{
  printf("block_child\n");
  ChildScheduler *sched = (ChildScheduler*)arg;
  sched->start_context = context;
}

static void child_main(void *arg)
{
  printf("child_main start\n");
  ChildScheduler child_sched;
  lithe_sched_enter(&child_sched);
  lithe_context_block(block_child, &child_sched);
  lithe_sched_exit();
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
