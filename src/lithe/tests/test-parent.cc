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
  void start(void *arg);
  void vcore_enter();
  void task_runnable(lithe_task_t *task); 

 public:
  lithe_task_t *start_task;

  ChildScheduler();
  ~ChildScheduler() {}
};

ChildScheduler::ChildScheduler()
{
  this->start_task = NULL;
}

void ChildScheduler::task_runnable(lithe_task_t *task) 
{
  printf("ChildScheduler::task_runnable\n");
  assert(task == this->start_task);
  lithe_task_run(task);
}

void ChildScheduler::vcore_enter()
{
  printf("ChildScheduler::vcore_enter\n");
  lithe_task_unblock(this->start_task);
}

static void block_child(lithe_task_t *task, void *arg)
{
  printf("block_child\n");
  ChildScheduler *sched = (ChildScheduler*)arg;
  sched->start_task = task;
}

void ChildScheduler::start(void *arg)
{
  printf("ChildScheduler::start start\n");
  lithe_task_block(block_child, this);
  printf("ChildScheduler::start finish\n");
}

void child_main(void *arg)
{
  printf("child_main start\n");
  /* Start a child scheduler: Blocks until scheduler finishes */
  ChildScheduler child_sched;
  lithe_sched_start(&Scheduler::funcs, &child_sched, NULL);
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
  void start(void *arg);
  void vcore_enter();
  int vcore_request(lithe_sched_t *child, int k);
  void child_started(lithe_sched_t *child);
  void child_finished(lithe_sched_t *child);
  void task_runnable(lithe_task_t *task);

 public:
  int lock;
  int vcores;
  int children_expected;
  int children_started;
  int children_finished;
  sched_vcore_request_list_t needy_children;
  lithe_task_t *start_task;

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
  this->start_task = NULL;
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

void RootScheduler::child_started(lithe_sched_t *child)
{
  printf("RootScheduler::child_started\n");
  spinlock_lock(&this->lock);
    this->children_started++;
  spinlock_unlock(&this->lock);
}

void RootScheduler::child_finished(lithe_sched_t *child)
{
  printf("RootScheduler::child_finished\n");
  spinlock_lock(&this->lock);
    this->children_finished++;
  spinlock_unlock(&this->lock);
}

void RootScheduler::task_runnable(lithe_task_t *task) 
{
  assert(task == this->start_task);
  lithe_task_run(task);
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
      lithe_task_t *task = lithe_task_create(NULL, child_main, NULL);
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
      lithe_task_unblock(this->start_task);
      break;
    }
  }
}

static void block_root(lithe_task_t *task, void *arg)
{
  printf("block_root\n");
  RootScheduler *sched = (RootScheduler *)arg;
  sched->start_task = task;
}

void RootScheduler::start(void *arg)
{
  printf("RootScheduler::start start\n");
  spinlock_lock(&this->lock);
    this->vcores = lithe_vcore_request(max_vcores()) + 1;
    this->children_expected = this->vcores;
    printf("children_expected: %d\n", this->children_expected);
  spinlock_unlock(&this->lock);
  lithe_task_block(block_root, this);
  printf("RootScheduler::start finish\n");
}

int main()
{
  printf("main start\n");
  /* Start the root scheduler: Blocks until scheduler finishes */
  RootScheduler root_sched;
  lithe_sched_start(&Scheduler::funcs, &root_sched, NULL);
  printf("main finish\n");
  return 0;
}

