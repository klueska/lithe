#include <stdio.h>
#include <unistd.h>

#include <lithe/lithe.h>
#include <lithe/lithe_mutex.h>
#include <lithe/fatal.h>


lithe_mutex_t mutex;
int count = 4;

void work()
{
  lithe_mutex_lock(&mutex);
  {
    lithe_task_t *task;
    if (lithe_task_get(&task) != 0) {
      fatal("could not get task");
    }
    printf("task 0x%x in critical section (count = %d)\n", (unsigned int)task, --count);
    sleep(1);
  }
  lithe_mutex_unlock(&mutex);
}


void __endenter(lithe_task_t *task, void *arg)
{
  printf("__endenter\n");
  free(task->uth.uc.uc_stack.ss_sp);
  lithe_task_destroy(task);
  free(task);
  lithe_sched_yield();
}


void __beginenter(void *__this)
{
  printf("__beginenter\n");
  work();
  lithe_task_block(__endenter, NULL);
}


void enter(void *__this)
{
  printf("enter\n");
  lithe_task_t *task = (lithe_task_t *) malloc(sizeof(lithe_task_t));
  int size = 4096;
  void *stack = malloc(size);
  lithe_task_init(task, stack, size);
  lithe_task_do(task, __beginenter, __this);
}


void yield(void *__this, lithe_sched_t *child)
{
  fatal("unimplemented");
}


void reg(void *__this, lithe_sched_t *sched)
{
  fatal("unimplemented");
}


void unreg(void *__this, lithe_sched_t *sched)
{
  fatal("unimplemented");
}


void request(void *__this, lithe_sched_t *sched, int k)
{
  fatal("unimplemented");
}


void unblock(void *__this, lithe_task_t *task)
{
  fatal("unimplemented");
}


static const lithe_sched_funcs_t funcs = {
  .enter   = enter,
  .yield   = yield,
  .reg     = reg,
  .unreg   = unreg,
  .request = request,
  .unblock = unblock,
};


void __endstart(lithe_task_t *task, void *arg)
{
  free(task->uth.uc.uc_stack.ss_sp);
  lithe_task_destroy(task);
  free(task);
  lithe_sched_unregister();
}


void __beginstart(void *arg)
{
  lithe_sched_request(1);
  work();
  lithe_task_block(__endstart, NULL);
}


void start(void *arg)
{
  lithe_task_t *task = (lithe_task_t *) malloc(sizeof(lithe_task_t));
  int size = 4096;
  void *stack = malloc(size);
  lithe_task_init(task, stack, size);
  lithe_task_do(task, __beginstart, NULL);
}


int main(int argc, char **argv)
{
  printf("main start\n");
  lithe_mutex_init(&mutex);
  lithe_sched_register(&funcs, NULL, start, NULL);
  printf("main finish\n");
  return 0;
}
