#include <stdio.h>
#include <unistd.h>

#include <spinlock.h>
#include <lithe/lithe.h>
#include <lithe/defaults.h>
#include <lithe/lithe_mutex.h>
#include <lithe/fatal.h>

/* The root scheduler itself */
typedef struct root_sched {
  lithe_sched_t sched;

  int task_count;
  lithe_mutex_t mutex;
  int qlock;
  struct task_deque taskq;
} root_sched_t;

static void root_sched_ctor(root_sched_t* sched)
{
  sched->task_count = 0;
  lithe_mutex_init(&sched->mutex);
  spinlock_init(&sched->qlock);
  task_deque_init(&sched->taskq);
}

static void root_sched_dtor(root_sched_t* sched)
{
}

/* Scheduler functions */
static void root_vcore_enter(lithe_sched_t *__this);
static lithe_task_t* root_task_create(lithe_sched_t *__this, void *udata);
static void root_task_exit(lithe_sched_t *__this, lithe_task_t *task);
static void root_task_runnable(lithe_sched_t *__this, lithe_task_t *task);

static const lithe_sched_funcs_t root_sched_funcs = {
  .vcore_request   = __vcore_request_default,
  .vcore_enter     = root_vcore_enter,
  .vcore_return    = __vcore_return_default,
  .child_entered   = __child_entered_default,
  .child_exited    = __child_exited_default,
  .task_create     = __task_create_default,
  .task_destroy    = __task_destroy_default,
  .task_runnable   = root_task_runnable,
  .task_yield      = __task_yield_default
};

static void root_vcore_enter(lithe_sched_t *__this)
{
  root_sched_t *sched = (root_sched_t *)__this;
  lithe_task_t *task = NULL;

  spinlock_lock(&sched->qlock);
    task_deque_dequeue(&sched->taskq, &task);
  spinlock_unlock(&sched->qlock);

  if(task == NULL)
    lithe_vcore_yield();
  else 
    lithe_task_run(task);
}

static void root_task_runnable(lithe_sched_t *__this, lithe_task_t *task)
{
  root_sched_t *sched = (root_sched_t *)__this;
  spinlock_lock(&sched->qlock);
    task_deque_enqueue(&sched->taskq, task);
    lithe_vcore_request(max_vcores());
  spinlock_unlock(&sched->qlock);
}


void work(void *arg)
{
  root_sched_t *sched = (root_sched_t*)arg;
  lithe_mutex_lock(&sched->mutex);
  {
    lithe_task_t *task = lithe_task_self();
    printf("task 0x%x in critical section (count = %d)\n", 
      (unsigned int)(unsigned long)task, --sched->task_count);
  }
  lithe_mutex_unlock(&sched->mutex);
  lithe_task_exit();
}


void root_run(int task_count)
{
  printf("root_run start\n");
  root_sched_t *sched = (root_sched_t*)lithe_sched_current();
  /* Create a bunch of worker tasks */
  sched->task_count = task_count;
  for(int i=0; i < sched->task_count; i++) {
    lithe_task_t *task  = lithe_task_create(NULL, work, sched);
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
  root_sched_t root_sched;
  root_sched_ctor(&root_sched);
  lithe_sched_enter(&root_sched_funcs, (lithe_sched_t*)&root_sched);
  root_run(150000);
  lithe_sched_exit();
  root_sched_dtor(&root_sched);
  printf("main finish\n");
  return 0;
}

