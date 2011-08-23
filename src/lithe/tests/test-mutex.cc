#include <stdio.h>
#include <unistd.h>

#include <spinlock.h>
#include <lithe/lithe.hh>
#include <lithe/defaults.h>
#include <lithe/lithe_mutex.h>
#include <lithe/fatal.h>

using namespace lithe;

class RootScheduler : public Scheduler {
 protected:
  void vcore_enter();
  void context_runnable(lithe_context_t *context);

 public:
  unsigned int context_count;
  lithe_mutex_t mutex;
  int qlock;
  struct context_deque contextq;

  RootScheduler();
  ~RootScheduler() {}
};

RootScheduler::RootScheduler()
{ 
  lithe_mutex_init(&this->mutex);
  spinlock_init(&this->qlock);
  context_deque_init(&this->contextq);
}

void RootScheduler::vcore_enter()
{
  lithe_context_t *context = NULL;

  spinlock_lock(&this->qlock);
    context_deque_dequeue(&this->contextq, &context);
  spinlock_unlock(&this->qlock);

  if(context == NULL)
    lithe_vcore_yield();
  else 
    lithe_context_run(context);
}

void RootScheduler::context_runnable(lithe_context_t *context)
{
  spinlock_lock(&this->qlock);
    context_deque_enqueue(&this->contextq, context);
    lithe_vcore_request(max_vcores());
  spinlock_unlock(&this->qlock);
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
  for(unsigned int i=0; i < sched->context_count; i++) {
    lithe_context_t *context  = lithe_context_create(NULL, work, (void*)sched);
    context_deque_enqueue(&sched->contextq, context);
  }

  /* Start up some more vcores to do our work for us */
  lithe_vcore_request(max_vcores());

  /* Wait for all the workers to run */
  while(1) {
    lithe_mutex_lock(&sched->mutex);
      if(sched->context_count == 0)
        break;
    lithe_mutex_unlock(&sched->mutex);
  }
  lithe_mutex_unlock(&sched->mutex);
  printf("root_run finish\n");
}

int main(int argc, char **argv)
{
  printf("main start\n");
  RootScheduler root_sched;
  lithe_sched_enter(&Scheduler::funcs, &root_sched);
  root_run(150000);
  lithe_sched_exit();
  printf("main finish\n");
  return 0;
}

