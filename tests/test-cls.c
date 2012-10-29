#include <stdio.h>
#include <unistd.h>

#include <parlib/parlib.h>
#include <src/lithe.h>
#include <src/mutex.h>
#include <src/defaults.h>

#define NUM_CLSKEYS 10
typedef struct root_sched {
  lithe_sched_t sched;

  unsigned int context_count;
  lithe_mutex_t mutex;
  mcs_lock_t qlock;
  struct lithe_context_deque contextq;
  lithe_clskey_t *clskeys[NUM_CLSKEYS];
} root_sched_t;

/* Scheduler functions */
static void root_hart_enter(lithe_sched_t *__this);
static void root_enqueue_task(lithe_sched_t *__this, lithe_context_t *context);
static void root_context_exit(lithe_sched_t *__this, lithe_context_t *context);
static void destroy_cls(lithe_clskey_t *key);

static const lithe_sched_funcs_t root_sched_funcs = {
  .hart_request         = __hart_request_default,
  .hart_enter           = root_hart_enter,
  .hart_return          = __hart_return_default,
  .child_enter          = __child_enter_default,
  .child_exit           = __child_exit_default,
  .context_block        = __context_block_default,
  .context_unblock      = root_enqueue_task,
  .context_yield        = root_enqueue_task,
  .context_exit         = root_context_exit
};

static void root_sched_ctor(root_sched_t* sched)
{
  sched->sched.funcs = &root_sched_funcs;
  sched->context_count = 0;
  lithe_mutex_init(&sched->mutex, NULL);
  mcs_lock_init(&sched->qlock);
  lithe_context_deque_init(&sched->contextq);
  for(int i=0; i<NUM_CLSKEYS; i++) {
    sched->clskeys[i] = lithe_clskey_create(destroy_cls);
  }
}

static void root_sched_dtor(root_sched_t *sched)
{
  for(int i=0; i<NUM_CLSKEYS; i++) {
    lithe_clskey_delete(sched->clskeys[i]);
  }
}

static void root_hart_enter(lithe_sched_t *__this)
{
  root_sched_t *sched = (root_sched_t *)__this;
  lithe_context_t *context = NULL;

  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&sched->qlock, &qnode);
    lithe_context_deque_dequeue(&sched->contextq, &context);
  mcs_lock_unlock(&sched->qlock, &qnode);

  if(context == NULL)
    lithe_hart_yield();
  else
    lithe_context_run(context);
}

static void root_enqueue_task(lithe_sched_t *__this, lithe_context_t *context)
{
  root_sched_t *sched = (root_sched_t *)__this;
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&sched->qlock, &qnode);
    lithe_context_deque_enqueue(&sched->contextq, context);
    lithe_hart_request(max_harts()-num_harts());
  mcs_lock_unlock(&sched->qlock, &qnode);
}

static void root_context_exit(lithe_sched_t *__this, lithe_context_t *context)
{
  assert(context);
  lithe_context_cleanup(context);
  __lithe_context_destroy_default(context, true);
}

static void destroy_cls(lithe_clskey_t *key) {
  root_sched_t *sched = (root_sched_t*)lithe_sched_current();
  void *cls = lithe_context_get_cls(key);
  
  lithe_mutex_lock(&sched->mutex);
  printf("Context %p freeing cls %p.\n", lithe_context_self(), cls);
  free(cls);
  lithe_mutex_unlock(&sched->mutex);
}

static void work(void *arg)
{
  root_sched_t *sched = (root_sched_t*)arg;

  long *cls_value[NUM_CLSKEYS];
  for(int i=0; i<NUM_CLSKEYS; i++) {
    cls_value[i] = malloc(sizeof(long));
    *cls_value[i] = (long)lithe_context_self() + i;
    lithe_context_set_cls(sched->clskeys[i], cls_value[i]);
  }
  lithe_mutex_lock(&sched->mutex);
  long self = (long)lithe_context_self();
  printf("In context %p (%ld)\n", (void*)self, self);
  for(int i=0; i<NUM_CLSKEYS; i++) {
    long *value = lithe_context_get_cls(sched->clskeys[i]);
    printf("  cls_value[%d] = %ld\n", i, *value);
  }
  sched->context_count--;
  lithe_mutex_unlock(&sched->mutex);
}

static void root_run(int context_count)
{
  printf("root_run start\n");
  root_sched_t *sched = (root_sched_t*)lithe_sched_current();
  /* Create a bunch of worker contexts */
  sched->context_count = context_count;
  for(unsigned int i=0; i < context_count; i++) {
    lithe_context_t *context = __lithe_context_create_default(true);
    lithe_context_init(context, work, (void*)sched);
    lithe_context_deque_enqueue(&sched->contextq, context);
  }

  /* Start up some more harts to do our work for us */
  lithe_hart_request(max_harts()-num_harts());

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

int main()
{
  printf("main start\n");
  root_sched_t root_sched;
  root_sched_ctor(&root_sched);
  lithe_sched_enter((lithe_sched_t*)&root_sched);
  root_run(1000);
  lithe_sched_exit();
  root_sched_dtor(&root_sched);
  printf("main finish\n");
  return 0;
}
