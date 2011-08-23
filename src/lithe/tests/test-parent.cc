#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <queue.h>
#include <spinlock.h>
#include <lithe/lithe.hh>
#include <lithe/deque.h>
#include <lithe/defaults.h>
#include <ht/atomic.h>

using namespace lithe;

class ChildScheduler : public Scheduler {
 protected:
  void vcore_enter();
  void context_runnable(lithe_context_t *context); 

 public:
  lithe_context_t *start_context;

  ChildScheduler();
  ~ChildScheduler() {}
};

ChildScheduler::ChildScheduler()
{
  this->start_context = NULL;
}

void ChildScheduler::context_runnable(lithe_context_t *context) 
{
  printf("ChildScheduler::context_runnable\n");
  assert(context == this->start_context);
  lithe_context_run(context);
}

void ChildScheduler::vcore_enter()
{
  printf("ChildScheduler::vcore_enter\n");
  lithe_context_unblock(this->start_context);
}

static void block_child(lithe_context_t *context, void *arg)
{
  printf("block_child\n");
  ChildScheduler *sched = (ChildScheduler*)arg;
  sched->start_context = context;
}

static void child_run()
{
  printf("child_run start\n");
  ChildScheduler *sched = (ChildScheduler*)lithe_sched_current();
  lithe_context_block(block_child, sched);
  printf("child_run finish\n");
}

void child_main(void *arg)
{
  printf("child_main start\n");
  /* Start a child scheduler: Blocks until scheduler finishes */
  ChildScheduler child_sched;
  lithe_sched_enter(&Scheduler::funcs, &child_sched);
  child_run();
  lithe_sched_exit();
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

class RootScheduler : public Scheduler {
 protected:
  void vcore_enter();
  int vcore_request(lithe_sched_t *child, int k);
  void child_entered(lithe_sched_t *child);
  void child_exited(lithe_sched_t *child);
  void context_runnable(lithe_context_t *context);

 public:
  int lock;
  int vcores;
  int children_expected;
  int children_started;
  int children_finished;
  sched_vcore_request_list_t needy_children;
  lithe_context_t *start_context;

  RootScheduler();
  ~RootScheduler() {}
};

RootScheduler::RootScheduler()
{
  spinlock_init(&this->lock);
  this->children_expected = 0;
  this->children_started = 0;
  this->children_finished = 0;
  LIST_INIT(&this->needy_children);
  this->start_context = NULL;
}

int RootScheduler::vcore_request(lithe_sched_t *child, int k)
{
  printf("RootScheduler::vcore_request\n");
  sched_vcore_request_t *req = (sched_vcore_request_t*)malloc(sizeof(sched_vcore_request_t));
  req->sched = child;
  req->num_vcores = k;
  spinlock_lock(&this->lock);
    LIST_INSERT_HEAD(&this->needy_children, req, link);
  spinlock_unlock(&this->lock);
  lithe_vcore_request(k);
  return k;
}

void RootScheduler::child_entered(lithe_sched_t *child)
{
  printf("RootScheduler::child_entered\n");
  spinlock_lock(&this->lock);
    this->children_started++;
  spinlock_unlock(&this->lock);
}

void RootScheduler::child_exited(lithe_sched_t *child)
{
  printf("RootScheduler::child_exited\n");
  spinlock_lock(&this->lock);
    this->children_finished++;
  spinlock_unlock(&this->lock);
}

void RootScheduler::context_runnable(lithe_context_t *context) 
{
  assert(context == this->start_context);
  lithe_context_run(context);
}

void RootScheduler::vcore_enter()
{
  printf("RootScheduler::vcore_enter\n");

  enum {
    STATE_CREATE,
    STATE_GRANT,
    STATE_YIELD,
    STATE_COMPLETE,
  };

  uint8_t state = 0;
  lithe_sched_t *child = NULL;
  spinlock_lock(&this->lock);
    if(this->children_started < this->children_expected)
      state = STATE_CREATE;
    else if(!LIST_EMPTY(&this->needy_children)) {
      state = STATE_GRANT;
      sched_vcore_request_t *req = LIST_FIRST(&this->needy_children);
      child = req->sched;
      req->num_vcores--;
      if(req->num_vcores == 0) {
        LIST_REMOVE(req, link);
        free(req);
      }
    }
    else if(this->vcores > 1) {
      this->vcores--;
      state = STATE_YIELD;
    }
    else
      state = STATE_COMPLETE;
  spinlock_unlock(&this->lock);

  switch(state) {
    case STATE_CREATE: {
      lithe_context_t *context = lithe_context_create(NULL, child_main, NULL);
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
      lithe_context_unblock(this->start_context);
      break;
    }
  }
}

static void block_root(lithe_context_t *context, void *arg)
{
  printf("block_root\n");
  RootScheduler *sched = (RootScheduler *)arg;
  sched->start_context = context;
}

static void root_run()
{
  printf("root_run start\n");
  RootScheduler *sched = (RootScheduler*)lithe_sched_current();
  spinlock_lock(&sched->lock);
    sched->vcores = lithe_vcore_request(max_vcores()) + 1;
    sched->children_expected = sched->vcores;
    printf("children_expected: %d\n", sched->children_expected);
  spinlock_unlock(&sched->lock);
  lithe_context_block(block_root, sched);
  printf("root_run finish\n");
}

int main()
{
  printf("main start\n");
  /* Start the root scheduler: Blocks until scheduler finishes */
  RootScheduler root_sched;
  lithe_sched_enter(&Scheduler::funcs, &root_sched);
  root_run();
  lithe_sched_exit();
  printf("main finish\n");
  return 0;
}

