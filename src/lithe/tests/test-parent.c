#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <parlib/parlib.h>
#include <parlib/mcs.h>
#include <parlib/queue.h>
#include <lithe/lithe.h>
#include <lithe/deque.h>
#include <lithe/defaults.h>

typedef struct child_sched {
  lithe_sched_t sched;
  lithe_context_t *start_context;
} child_sched_t;

static void child_vcore_enter(lithe_sched_t *__this);
static void child_context_unblock(lithe_sched_t *__this, lithe_context_t *context); 

static const lithe_sched_funcs_t child_funcs = {
  .vcore_request         = __vcore_request_default,
  .vcore_enter           = child_vcore_enter,
  .vcore_return          = __vcore_return_default,
  .child_entered         = __child_entered_default,
  .child_exited          = __child_exited_default,
  .context_block         = __context_block_default,
  .context_unblock       = child_context_unblock,
  .context_yield         = __context_yield_default,
  .context_exit          = __context_exit_default,
};

static void child_sched_ctor(child_sched_t *sched)
{
  sched->sched.funcs = &child_funcs;
  sched->start_context = NULL;
}

static void child_sched_dtor(child_sched_t *sched)
{
}

static void child_context_unblock(lithe_sched_t *__this, lithe_context_t *context) 
{
  printf("child_context_runnable\n");
  child_sched_t *sched = (child_sched_t *)__this;
  assert(context == sched->start_context);
  lithe_context_run(context);
}

void child_vcore_enter(lithe_sched_t *__this) 
{
  printf("child_enter\n");
  child_sched_t *sched = (child_sched_t *)__this;
  lithe_context_unblock(sched->start_context);
}

static void block_child(lithe_context_t *context, void *arg)
{
  printf("block_child\n");
  child_sched_t *sched = (child_sched_t*)lithe_sched_current();
  sched->start_context = context;
}

static void child_run()
{
  printf("child_run start\n");
  lithe_context_block(block_child, NULL);
  printf("child_run finish\n");
}

void child_main(void *arg)
{
  printf("child_main start\n");
  /* Start a child scheduler: Blocks until scheduler finishes */
  child_sched_t child_sched;
  child_sched_ctor(&child_sched);
  lithe_sched_enter((lithe_sched_t*)&child_sched);
  child_run();
  lithe_sched_exit();
  child_sched_dtor(&child_sched);
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
  mcs_lock_t lock;
  int vcores;
  int children_expected;
  int children_started;
  int children_finished;
  sched_vcore_request_list_t needy_children;
  lithe_context_t *start_context;
} root_sched_t;

int root_vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k);
static void root_vcore_enter(lithe_sched_t *this);
void root_child_entered(lithe_sched_t *__this, lithe_sched_t *child);
void root_child_exited(lithe_sched_t *__this, lithe_sched_t *child);
static void root_context_unblock(lithe_sched_t *__this, lithe_context_t *context);
static void root_context_exit(lithe_sched_t *__this, lithe_context_t *context);

static const lithe_sched_funcs_t root_funcs = {
  .vcore_request         = root_vcore_request,
  .vcore_enter           = root_vcore_enter,
  .vcore_return          = __vcore_return_default,
  .child_entered         = root_child_entered,
  .child_exited          = root_child_exited,
  .context_block         = __context_block_default,
  .context_unblock       = root_context_unblock,
  .context_yield         = __context_yield_default,
  .context_exit          = root_context_exit
};

static void root_sched_ctor(root_sched_t *sched)
{
  sched->sched.funcs = &root_funcs;
  mcs_lock_init(&sched->lock);
  sched->children_expected = 0;
  sched->children_started = 0;
  sched->children_finished = 0;
  LIST_INIT(&sched->needy_children);
  sched->start_context = NULL;
}

static void root_sched_dtor(root_sched_t *sched)
{
}

int root_vcore_request(lithe_sched_t *__this, lithe_sched_t *child, int k)
{
  printf("root_vcore_request\n");
  root_sched_t *sched = (root_sched_t *)__this;
  sched_vcore_request_t *req = malloc(sizeof(sched_vcore_request_t));
  req->sched = child;
  req->num_vcores = k;
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&sched->lock, &qnode);
    LIST_INSERT_HEAD(&sched->needy_children, req, link);
  mcs_lock_unlock(&sched->lock, &qnode);
  lithe_vcore_request(k);
  return k;
}

void root_child_entered(lithe_sched_t *__this, lithe_sched_t *child)
{
  printf("root_child_entered\n");
  root_sched_t *sched = (root_sched_t *)__this;
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&sched->lock, &qnode);
    sched->children_started++;
  mcs_lock_unlock(&sched->lock, &qnode);
}

void root_child_exited(lithe_sched_t *__this, lithe_sched_t *child)
{
  printf("root_child_exited\n");
  root_sched_t *sched = (root_sched_t *)__this;
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&sched->lock, &qnode);
    sched->children_finished++;
  mcs_lock_unlock(&sched->lock, &qnode);
}

static void root_context_unblock(lithe_sched_t *__sched, lithe_context_t *context) 
{
  printf("root_context_unblock\n");
  root_sched_t *sched = (root_sched_t *)__sched;
  assert(context == sched->start_context);
  lithe_context_run(context);
}

void root_context_exit(lithe_sched_t *__sched, lithe_context_t *context) 
{
  printf("root_context_exit\n");
  assert(context);
  lithe_context_cleanup(context);
  __lithe_context_destroy_default(context, true);
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
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&sched->lock, &qnode);
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
  mcs_lock_unlock(&sched->lock, &qnode);

  switch(state) {
    case STATE_CREATE: {
      lithe_context_t *context = __lithe_context_create_default(true);
      lithe_context_init(context, child_main, NULL);
      lithe_context_run(context);
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
      lithe_context_unblock(sched->start_context);
      break;
    }
  }
}

static void block_root(lithe_context_t *context, void *arg)
{
  printf("block_root\n");
  root_sched_t *sched = (root_sched_t*)lithe_sched_current();
  sched->start_context = context;
}

static void root_run()
{
  printf("root_run start\n");
  root_sched_t *sched = (root_sched_t*)lithe_sched_current();
  mcs_lock_qnode_t qnode = {0};
  mcs_lock_lock(&sched->lock, &qnode);
    sched->vcores = lithe_vcore_request(max_vcores()) + 1;
    sched->children_expected = sched->vcores;
    printf("children_expected: %d\n", sched->children_expected);
  mcs_lock_unlock(&sched->lock, &qnode);
  lithe_context_block(block_root, NULL);
  printf("root_run finish\n");
}

int main()
{
  printf("root_main start\n");
  /* Start the root scheduler: Blocks until scheduler finishes */
  root_sched_t root_sched;
  root_sched_ctor(&root_sched);
  lithe_sched_enter((lithe_sched_t*)&root_sched);
  root_run();
  lithe_sched_exit();
  root_sched_dtor(&root_sched);
  printf("root_main finish\n");
  return 0;
}

