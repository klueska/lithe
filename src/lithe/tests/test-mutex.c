#include <stdio.h>
#include <unistd.h>

#include <spinlock.h>
#include <lithe/lithe.h>
#include <lithe/defaults.h>
#include <lithe/lithe_mutex.h>
#include <lithe/fatal.h>

int task_count = 150000;
lithe_mutex_t mutex;

/* The root scheduler itself */
typedef struct root_sched {
  lithe_sched_t sched;
  int qlock;
  struct task_deque taskq;
} root_sched_t;

/* Scheduler functions */
static lithe_sched_t *root_construct(void *arg);
static void root_destroy(lithe_sched_t *__this);
static void root_start(lithe_sched_t *__this);
static void root_vcore_enter(lithe_sched_t *__this);
static lithe_task_t* root_task_create(lithe_sched_t *__this, void *udata);
static void root_task_exit(lithe_sched_t *__this, lithe_task_t *task);
static void root_task_runnable(lithe_sched_t *__this, lithe_task_t *task);

static const lithe_sched_funcs_t root_sched_funcs = {
  .construct       = root_construct,
  .destroy         = __destroy_default,
  .start           = root_start,
  .vcore_request   = __vcore_request_default,
  .vcore_enter     = root_vcore_enter,
  .vcore_return    = __vcore_return_default,
  .child_started   = __child_started_default,
  .child_finished  = __child_finished_default,
  .task_create     = root_task_create,
  .task_yield      = __task_yield_default,
  .task_exit       = root_task_exit,
  .task_runnable   = root_task_runnable
};

static lithe_sched_t *root_construct(void *__sched)
{
  root_sched_t *sched = (root_sched_t*)__sched;
  spinlock_init(&sched->qlock);
  task_deque_init(&sched->taskq);
  return (lithe_sched_t*)sched;
}


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


static lithe_task_t* root_task_create(lithe_sched_t *__this, void *udata)
{
  lithe_task_t *task = malloc(sizeof(lithe_task_t));
  task->stack.size = 4096;
  task->stack.sp = malloc(task->stack.size);
  assert(task->stack.sp);
  return task;
}


static void root_task_exit(lithe_sched_t *__this, lithe_task_t *task)
{
  free(task->stack.sp);
}


static void root_task_runnable(lithe_sched_t *__this, lithe_task_t *task)
{
  root_sched_t *sched = (root_sched_t *)__this;
  spinlock_lock(&sched->qlock);
    task_deque_enqueue(&sched->taskq, task);
    lithe_vcore_request(max_vcores());
  spinlock_unlock(&sched->qlock);
}


void work()
{
  lithe_mutex_lock(&mutex);
  {
    lithe_task_t *task = lithe_task_self();
    printf("task 0x%x in critical section (count = %d)\n", 
      (unsigned int)(unsigned long)task, --task_count);
  }
  lithe_mutex_unlock(&mutex);
}


void root_start(lithe_sched_t *__this)
{
  printf("root_start start\n");
  root_sched_t *sched = (root_sched_t*)__this;
  lithe_mutex_init(&mutex);
  /* Create a bunch of worker tasks */
  for(int i=0; i < task_count; i++) {
    lithe_task_t *task  = lithe_task_create(work, NULL);
    task_deque_enqueue(&sched->taskq, task);
  }

  /* Start up some more vcores to do our work for us */
  lithe_vcore_request(max_vcores());

  /* Wait for all the workers to run */
  while(1) {
    lithe_mutex_lock(&mutex);
      if(task_count == 0)
        break;
    lithe_mutex_unlock(&mutex);
  }
  lithe_mutex_unlock(&mutex);
  printf("root_start finish\n");
}

int main(int argc, char **argv)
{
  printf("main start\n");
  root_sched_t root_sched;
  lithe_sched_start(&root_sched_funcs, &root_sched);
  printf("main finish\n");
  return 0;
}

