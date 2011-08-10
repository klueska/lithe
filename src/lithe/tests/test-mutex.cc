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

void root_run(int task_count)
{
  printf("root_run start\n");
  /* Create a bunch of worker tasks */
  RootScheduler *sched = (RootScheduler*)lithe_sched_current();
  sched->task_count = task_count;
  for(unsigned int i=0; i < sched->task_count; i++) {
    lithe_task_t *task  = lithe_task_create(NULL, work, (void*)sched);
    task_deque_enqueue(&sched->taskq, task);
  }

  /* Start up some more vcores to do our work for us */
  lithe_vcore_request(max_vcores());

  /* Wait for all the workers to run */
  while(1) {
    lithe_mutex_lock(&sched->mutex);
      if(sched->task_count == 0)
        break;
    lithe_mutex_unlock(&sched->mutex);
  }
  lithe_mutex_unlock(&sched->mutex);
  printf("root_run finish\n");
}

int main(int argc, char **argv)
{
  printf("main start\n");
  RootScheduler sched;
  lithe_sched_enter(&Scheduler::funcs, &sched);
  root_run(150000);
  lithe_sched_exit();
  printf("main finish\n");
  return 0;
}

