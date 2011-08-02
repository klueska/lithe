#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <queue.h>
#include <spinlock.h>
#include <lithe/lithe.h>
#include <lithe/deque.h>
#include <lithe/defaults.h>
#include <ht/atomic.h>

typedef struct child_sched {
  lithe_sched_t sched;
  lithe_task_t *start_task;
} child_sched_t;

static lithe_sched_t *child_construct(void *__sched);
static void child_start(lithe_sched_t *__this);
static void child_vcore_enter(lithe_sched_t *__this);
static void child_task_runnable(lithe_sched_t *__this, lithe_task_t *task); 

static const lithe_sched_funcs_t child_funcs = {
  .construct       = child_construct,
  .destroy         = __destroy_default,
  .start           = child_start,
  .vcore_request   = __vcore_request_default,
  .vcore_enter     = child_vcore_enter,
  .vcore_return    = __vcore_return_default,
  .child_started   = __child_started_default,
  .child_finished  = __child_finished_default,
  .task_create     = __task_create_default,
  .task_yield      = __task_yield_default,
  .task_exit       = __task_exit_default,
  .task_runnable   = child_task_runnable
};

static lithe_sched_t *child_construct(void *__sched)
{
  child_sched_t *sched = (child_sched_t*)__sched;
  sched->start_task = NULL;
  return (lithe_sched_t*)sched;
}

static void child_task_runnable(lithe_sched_t *__this, lithe_task_t *task) 
{
  printf("child_task_runnable\n");
  child_sched_t *sched = (child_sched_t *)__this;
  assert(task == sched->start_task);
  lithe_task_run(task);
}

void child_vcore_enter(lithe_sched_t *__this) 
{
  printf("child_enter\n");
  child_sched_t *sched = (child_sched_t *)__this;
  lithe_task_unblock(sched->start_task);
}

static void block_child(lithe_task_t *task, void *arg)
{
  printf("block_child\n");
  child_sched_t *sched = lithe_sched_current();
  sched->start_task = task;
}

static void child_start(lithe_sched_t *__this)
{
  printf("child_start start\n");
  lithe_task_block(block_child, NULL);
  printf("child_start finish\n");
}

void child_main()
{
  printf("child_main start\n");
  /* Start a child scheduler: Blocks until scheduler finishes */
  child_sched_t child_sched;
  lithe_sched_start(&child_funcs, &child_sched);
  printf("child_main finish\n");
}

struct sched_vcore_request {
  LIST_ENTRY(sched_vcore_request) link;
  lithe_sched_t *sched;
  int num_vcores;
}; 
LIST_HEAD(sched_vcore_request_list, sched_vcore_request);
typedef struct sched_vcore_request sched_vcore_request_t;
typedef struct sched_vcore_request_list sched_vcore_request_list_t;

typedef struct root_sched {
  lithe_sched_t sched;
  int lock;
  int vcores;
  int children_expected;
  int children_started;
  int children_finished;
  sched_vcore_request_list_t needy_children;
  lithe_task_t *start_task;
} root_sched_t;

static lithe_sched_t *root_construct(void *__sched);
static void root_destroy(lithe_sched_t *__this);
int root_vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k);
static void root_start(lithe_sched_t *__this);
static void root_vcore_enter(lithe_sched_t *this);
void root_child_started(lithe_sched_t *__this, lithe_sched_t *child);
void root_child_finished(lithe_sched_t *__this, lithe_sched_t *child);
static void root_task_runnable(lithe_sched_t *__this, lithe_task_t *task);

static const lithe_sched_funcs_t root_funcs = {
  .construct       = root_construct,
  .destroy         = __destroy_default,
  .start           = root_start,
  .vcore_request   = root_vcore_request,
  .vcore_enter     = root_vcore_enter,
  .vcore_return    = __vcore_return_default,
  .child_started   = root_child_started,
  .child_finished  = root_child_finished,
  .task_create     = __task_create_default,
  .task_yield      = __task_yield_default,
  .task_exit       = __task_exit_default,
  .task_runnable   = root_task_runnable
};

static lithe_sched_t *root_construct(void *__sched)
{
  root_sched_t *sched = (root_sched_t*)__sched;
  spinlock_init(&sched->lock);
  sched->children_expected = 0;
  sched->children_started = 0;
  sched->children_finished = 0;
  LIST_INIT(&sched->needy_children);
  sched->start_task = NULL;
  return (lithe_sched_t*)sched;
}

int root_vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k)
{
  printf("root_vcore_request\n");
  root_sched_t *sched = (root_sched_t *)__this;
  sched_vcore_request_t *req = malloc(sizeof(sched_vcore_request_t));
  req->sched = child;
  req->num_vcores = k;
  spinlock_lock(&sched->lock);
    LIST_INSERT_HEAD(&sched->needy_children, req, link);
  spinlock_unlock(&sched->lock);
  lithe_vcore_request(k);
  return k;
}

void root_child_started(lithe_sched_t *__this, lithe_sched_t *child)
{
  printf("root_child_started\n");
  root_sched_t *sched = (root_sched_t *)__this;
  spinlock_lock(&sched->lock);
    sched->children_started++;
  spinlock_unlock(&sched->lock);
}

void root_child_finished(lithe_sched_t *__this, lithe_sched_t *child)
{
  printf("root_child_finished\n");
  root_sched_t *sched = (root_sched_t *)__this;
  spinlock_lock(&sched->lock);
    sched->children_finished++;
  spinlock_unlock(&sched->lock);
}

static void root_task_runnable(lithe_sched_t *__sched, lithe_task_t *task) 
{
  root_sched_t *sched = (root_sched_t *)__sched;
  assert(task == sched->start_task);
  lithe_task_run(task);
}

static void root_vcore_enter(lithe_sched_t *__sched) 
{
  printf("root_vcore_enter\n");
  root_sched_t *sched = (root_sched_t*) __sched;

  enum {
    STATE_CREATE,
    STATE_GRANT,
    STATE_YIELD,
    STATE_COMPLETE,
  };

  uint8_t state = 0;
  lithe_sched_t *child = NULL;
  spinlock_lock(&sched->lock);
    if(sched->children_started < sched->children_expected)
      state = STATE_CREATE;
    else if(!LIST_EMPTY(&sched->needy_children)) {
      state = STATE_GRANT;
      sched_vcore_request_t *req = LIST_FIRST(&sched->needy_children);
      child = req->sched;
      req->num_vcores--;
      if(req->num_vcores == 0) {
        LIST_REMOVE(req, link);
        free(req);
      }
    }
    else if(sched->vcores > 1) {
      sched->vcores--;
      state = STATE_YIELD;
    }
    else
      state = STATE_COMPLETE;
  spinlock_unlock(&sched->lock);

  switch(state) {
    case STATE_CREATE: {
      lithe_task_t *task = lithe_task_create(child_main, NULL);
      lithe_task_run(task);
      break;
    }
    case STATE_GRANT: {
      lithe_vcore_grant(child);
      break;
    }
    case STATE_YIELD: {
      lithe_vcore_yield();
      break;
    }
    case STATE_COMPLETE: {
      lithe_task_unblock(sched->start_task);
      break;
    }
  }
}

static void block_root(lithe_task_t *task, void *arg)
{
  printf("block_root\n");
  root_sched_t *sched = lithe_sched_current();
  sched->start_task = task;
}

static void root_start(lithe_sched_t *__this)
{
  printf("root_start start\n");
  root_sched_t *sched = (root_sched_t*)__this;
  spinlock_lock(&sched->lock);
    sched->vcores = lithe_vcore_request(max_vcores()) + 1;
    sched->children_expected = sched->vcores;
    printf("children_expected: %d\n", sched->children_expected);
  spinlock_unlock(&sched->lock);
  lithe_task_block(block_root, NULL);
  printf("root_start finish\n");
}

int main()
{
  printf("root_main start\n");
  /* Start the root scheduler: Blocks until scheduler finishes */
  root_sched_t root_sched;
  lithe_sched_start(&root_funcs, &root_sched);
  printf("root_main finish\n");
  return 0;
}

