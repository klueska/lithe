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
  void start(void *arg);
  void vcore_enter();
  void task_runnable(lithe_task_t *task);

 public:
  unsigned int task_count;
  lithe_mutex_t mutex;
  int qlock;
  struct task_deque taskq;

  RootScheduler();
  ~RootScheduler() {}
};

RootScheduler::RootScheduler()
{ 
  lithe_mutex_init(&this->mutex);
  spinlock_init(&this->qlock);
  task_deque_init(&this->taskq);
}

void RootScheduler::vcore_enter()
{
  lithe_task_t *task = NULL;

  spinlock_lock(&this->qlock);
    task_deque_dequeue(&this->taskq, &task);
  spinlock_unlock(&this->qlock);

  if(task == NULL)
    lithe_vcore_yield();
  else 
    lithe_task_run(task);
}

void RootScheduler::task_runnable(lithe_task_t *task)
{
  spinlock_lock(&this->qlock);
    task_deque_enqueue(&this->taskq, task);
    lithe_vcore_request(max_vcores());
  spinlock_unlock(&this->qlock);
}

void work(void *arg)
{
  RootScheduler *sched = (RootScheduler*)arg;
  lithe_mutex_lock(&sched->mutex);
  {
    lithe_task_t *task = lithe_task_self();
    printf("task 0x%x in critical section (count = %d)\n", 
      (unsigned int)(unsigned long)task, --sched->task_count);
  }
  lithe_mutex_unlock(&sched->mutex);
}

void RootScheduler::start(void *arg)
{
  printf("RootScheduler start\n");
  /* Create a bunch of worker tasks */
  for(unsigned int i=0; i < this->task_count; i++) {
    lithe_task_t *task  = lithe_task_create(NULL, work, (void*)this);
    task_deque_enqueue(&this->taskq, task);
  }

  /* Start up some more vcores to do our work for us */
  lithe_vcore_request(max_vcores());

  /* Wait for all the workers to run */
  while(1) {
    lithe_mutex_lock(&this->mutex);
      if(this->task_count == 0)
        break;
    lithe_mutex_unlock(&this->mutex);
  }
  lithe_mutex_unlock(&this->mutex);
  printf("RootScheduler finish\n");
}

int main(int argc, char **argv)
{
  printf("main start\n");
  RootScheduler sched;
  sched.task_count = 150000;
  lithe_sched_start(&Scheduler::funcs, &sched, NULL);
  printf("main finish\n");
  return 0;
}

