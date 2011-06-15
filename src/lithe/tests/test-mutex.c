#include <stdio.h>
#include <unistd.h>

#include <spinlock.h>
#include <lithe/lithe.h>
#include <lithe/lithe_mutex.h>
#include <lithe/fatal.h>


lithe_mutex_t mutex;

int task_count = 100000;
struct task_deque taskq;
int qlock;

void __endwork(lithe_task_t *task, void *arg)
{
  free(task->stack.sp);
  lithe_task_destroy(task);
  lithe_sched_reenter();
}


void work()
{
  lithe_mutex_lock(&mutex);
  {
    lithe_task_t *task;
    if (lithe_task_get(&task) != 0) {
      fatal("could not get task");
    }
    printf("task 0x%x in critical section (count = %d)\n", (unsigned int)task, --task_count);
  }
  lithe_mutex_unlock(&mutex);
  lithe_task_block(__endwork, NULL);
}

void enter(void *__this)
{
  lithe_task_t *task = NULL;

  spinlock_lock(&qlock);
    task_deque_dequeue(&taskq, &task);
  spinlock_unlock(&qlock);

  if(task == NULL)
    lithe_sched_yield();
  else 
    lithe_task_run(task);
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
  spinlock_lock(&qlock);
    task_deque_enqueue(&taskq, task);
    lithe_sched_request(max_vcores());
  spinlock_unlock(&qlock);
}


static const lithe_sched_funcs_t funcs = {
  .enter   = enter,
  .yield   = yield,
  .reg     = reg,
  .unreg   = unreg,
  .request = request,
  .unblock = unblock,
};

void run_tasks()
{
  lithe_sched_request(max_vcores());

  while(1) {
    lithe_mutex_lock(&mutex);
      if(task_count == 0)
        break;
    lithe_mutex_unlock(&mutex);
  }
  lithe_mutex_unlock(&mutex);
  lithe_sched_unregister();
}

void start(void *arg)
{
  printf("start\n");
  /* Create a bunch of worker tasks */
  for(int i=0; i < task_count; i++) {
    lithe_task_t *task = NULL;
    lithe_task_stack_t stack = {malloc(4096), 4096};
    lithe_task_create(&task, work, NULL, &stack);
    task_deque_enqueue(&taskq, task);
  }
  /* Create an initial task to set everything in motion and run it */
  lithe_task_t *task = NULL;
  lithe_task_create(&task, run_tasks, NULL, NULL);
  lithe_task_run(task);
}


int main(int argc, char **argv)
{
  printf("main start\n");
  lithe_mutex_init(&mutex);
  task_deque_init(&taskq);
  spinlock_init(&qlock);
  lithe_sched_register(&funcs, NULL, start, NULL);
  printf("main finish\n");
  return 0;
}
