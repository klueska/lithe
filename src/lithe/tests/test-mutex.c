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

  int context_count;
  lithe_mutex_t mutex;
  int qlock;
  struct context_deque contextq;
} root_sched_t;

static void root_sched_ctor(root_sched_t* sched)
{
  sched->context_count = 0;
  lithe_mutex_init(&sched->mutex);
  spinlock_init(&sched->qlock);
  context_deque_init(&sched->contextq);
}

static void root_sched_dtor(root_sched_t* sched)
{
}

/* Scheduler functions */
static void root_vcore_enter(lithe_sched_t *__this);
static lithe_context_t* root_context_create(lithe_sched_t *__this, void *udata);
static void root_context_exit(lithe_sched_t *__this, lithe_context_t *context);
static void root_context_runnable(lithe_sched_t *__this, lithe_context_t *context);

static const lithe_sched_funcs_t root_sched_funcs = {
  .vcore_request         = __vcore_request_default,
  .vcore_enter           = root_vcore_enter,
  .vcore_return          = __vcore_return_default,
  .child_entered         = __child_entered_default,
  .child_exited          = __child_exited_default,
  .context_create        = __context_create_default,
  .context_destroy       = __context_destroy_default,
  .context_stack_create  = __context_stack_create_default,
  .context_stack_destroy = __context_stack_destroy_default,
  .context_runnable      = root_context_runnable,
  .context_yield         = __context_yield_default
};

static void root_vcore_enter(lithe_sched_t *__this)
{
  root_sched_t *sched = (root_sched_t *)__this;
  lithe_context_t *context = NULL;

  spinlock_lock(&sched->qlock);
    context_deque_dequeue(&sched->contextq, &context);
  spinlock_unlock(&sched->qlock);

  if(context == NULL)
    lithe_vcore_yield();
  else 
    lithe_context_run(context);
}

static void root_context_runnable(lithe_sched_t *__this, lithe_context_t *context)
{
  root_sched_t *sched = (root_sched_t *)__this;
  spinlock_lock(&sched->qlock);
    context_deque_enqueue(&sched->contextq, context);
    lithe_vcore_request(max_vcores());
  spinlock_unlock(&sched->qlock);
}


void work(void *arg)
{
  root_sched_t *sched = (root_sched_t*)arg;
  lithe_mutex_lock(&sched->mutex);
  {
    lithe_context_t *context = lithe_context_self();
    printf("context 0x%x in critical section (count = %d)\n", 
      (unsigned int)(unsigned long)context, --sched->context_count);
  }
  lithe_mutex_unlock(&sched->mutex);
  lithe_context_exit();
}


void root_run(int context_count)
{
  printf("root_run start\n");
  root_sched_t *sched = (root_sched_t*)lithe_sched_current();
  /* Create a bunch of worker contexts */
  sched->context_count = context_count;
  for(int i=0; i < sched->context_count; i++) {
    lithe_context_t *context  = lithe_context_create(NULL, work, sched);
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
  root_sched_t root_sched;
  root_sched_ctor(&root_sched);
  lithe_sched_enter(&root_sched_funcs, (lithe_sched_t*)&root_sched);
  root_run(150000);
  lithe_sched_exit();
  root_sched_dtor(&root_sched);
  printf("main finish\n");
  return 0;
}

