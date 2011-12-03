#include <stdio.h>
#include <unistd.h>

#include <parlib/mcs.h>
#include "lithe.hh"
#include "defaults.h"
#include "lithe_mutex.h"
#include "fatal.h"

using namespace lithe;

class RootScheduler : public Scheduler {
 protected:
  void hart_enter();
  void context_unblock(lithe_context_t *context);
  void context_yield(lithe_context_t *context);
  void context_exit(lithe_context_t *context);

 public:
  unsigned int context_count;
  lithe_mutex_t mutex;
  mcs_lock_t qlock;
  struct context_deque contextq;

  RootScheduler();
  ~RootScheduler() {}
};

RootScheduler::RootScheduler()
{  
  this->context_count = 0;
  lithe_mutex_init(&this->mutex);
  mcs_lock_init(&this->qlock);
  context_deque_init(&this->contextq);
}

void RootScheduler::hart_enter()
{
  lithe_context_t *context = NULL;

  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&this->qlock, &qnode);
    context_deque_dequeue(&this->contextq, &context);
  mcs_lock_unlock(&this->qlock, &qnode);

  if(context == NULL)
    lithe_hart_yield();
  else
    lithe_context_run(context);
}

void enqueue_task(RootScheduler *sched, lithe_context_t *context)
{
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&sched->qlock, &qnode);
    context_deque_enqueue(&sched->contextq, context);
    lithe_hart_request(limit_harts());
  mcs_lock_unlock(&sched->qlock, &qnode);
}

void RootScheduler::context_unblock(lithe_context_t *context)
{
  enqueue_task(this, context);
}

void RootScheduler::context_yield(lithe_context_t *context)
{
  enqueue_task(this, context);
}

void RootScheduler::context_exit(lithe_context_t *context)
{
  assert(context);
  lithe_context_cleanup(context);
  __lithe_context_destroy_default(context, true);
}

void work(void *arg)
{
  RootScheduler *sched = (RootScheduler*)arg;
  lithe_mutex_lock(&sched->mutex);
  {
    lithe_context_t *context = lithe_context_self();
    printf("context 0x%x in critical section (count = %d)\n", 
      (unsigned int)(unsigned long)context, --sched->context_count);
  }
  lithe_mutex_unlock(&sched->mutex);
}

void root_run(int context_count)
{
  printf("root_run start\n");
  /* Create a bunch of worker contexts */
  RootScheduler *sched = (RootScheduler*)lithe_sched_current();
  sched->context_count = context_count;
  for(unsigned int i=0; i < context_count; i++) {
    lithe_context_t *context = __lithe_context_create_default(true);
    lithe_context_init(context, work, (void*)sched);
    context_deque_enqueue(&sched->contextq, context);
  }

  /* Start up some more harts to do our work for us */
  lithe_hart_request(limit_harts());

  /* Wait for all the workers to run */
  while(1) {
    lithe_mutex_lock(&sched->mutex);
      if(sched->context_count == 0)
        break;
    lithe_mutex_unlock(&sched->mutex);
    lithe_context_yield();
  }
  lithe_mutex_unlock(&sched->mutex);
  printf("root_run finish\n");
}

int main(int argc, char **argv)
{
  printf("main start\n");
  RootScheduler root_sched;
  lithe_sched_enter(&root_sched);
  root_run(1500);
  lithe_sched_exit();
  printf("main finish\n");
  return 0;
}

