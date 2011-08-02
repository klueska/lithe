#include <stdio.h>
#include <unistd.h>

#include <lithe/lithe.hh>

using namespace lithe;

class MutexScheduler : public Scheduler {
 private:
  lithe_mutex_t mutex;

 protected:
  void work();
  void cleanup(lithe_task_t *task);

  virtual void enter();
  virtual void yield(lithe_sched_t *child);
  virtual void reg(lithe_sched_t *child);
  virtual void unreg(lithe_sched_t *child);
  virtual void request(lithe_sched_t *child, int k);
  virtual void unblock(lithe_task_t *task);

 public:
  MutexScheduler() { lithe_mutex_init(&this->mutex); }
  ~MutexScheduler() {}
};


void MutexScheduler::work()
{
  lithe_mutex_lock(&this->mutex);
  {
    lithe_task_t *task;
    if (lithe_get_task(&task) != 0) {
      fatal("could not get task");
    }
    printf("task 0x%x made i = %d\n", task, ++i);
  }
  lithe_mutex_unlock(&this->mutex);

  lithe_task_block(cleanup, NULL);
}


void MutexScheduler::cleanup(lithe_task_t *task)
{
  free(task->stack);
  free(task);
  lithe_yield();
}


void MutexScheduler::enter()
{
  size_t size = 2048;
  void *stack = malloc(size);

  lithe_task_t *task = malloc(sizeof(lithe_task_t));
  lithe_task_init(task, stack, size);
  lithe_task_trampoline(task, func);
}


void MutexScheduler::yield(lithe_sched_t *child)
{
  fatal("unimplemented");
}


void MutexScheduler::reg(lithe_sched_t *child)
{
  fatal("unimplemented");
}


void MutexScheduler::unreg(lithe_sched_t *child)
{
  fatal("unimplemented");
}


void MutexScheduler::request(lithe_sched_t *child, int k)
{
  fatal("unimplemented");
}


void MutexScheduler::unblock(lithe_task_t *task)
{
  fatal("unimplemented");
}


extern "C" {


int main(int argc, char **argv)
{
  printf("main start\n");

  MutexScheduler sched(&mutex);
  sched.doRegister();
  sched.doRequest(1);

  size_t size = 2048;
  void *stack = malloc(size);

  lithe_task_t task;
  lithe_task_init(&task, stack, size);
  lithe_task_do(&task, );

  sched.doUnregister();

  printf("main finish\n");

  return 0;
}
 
}
