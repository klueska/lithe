#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <queue.h>
#include <spinlock.h>
#include <lithe/lithe.h>
#include <lithe/deque.h>
#include <ht/atomic.h>

struct child_sched {
  LIST_ENTRY(child_sched) link;
  void *sched;
};
LIST_HEAD(child_sched_list, child_sched);
typedef struct child_sched child_sched_t;
typedef struct child_sched_list child_sched_list_t;

typedef struct root_sched {
  int lock;
  int expected_children;
  child_sched_list_t children;
  lithe_task_t *start_task;
} root_sched_t;

void child_enter(void *__this) 
{
  printf("child_enter\n");
  lithe_sched_unregister();
}

void child_yield(void *__this, lithe_sched_t *child)
{ 
  assert(false);
}

void child_reg(void *__this, lithe_sched_t *child) 
{
  assert(false);
}

void child_unreg(void *__this, lithe_sched_t *child) 
{
  assert(false);
}

int child_request(void *__this, lithe_sched_t *child, int k) 
{
  assert(false);
}

void child_unblock(void *__this, lithe_task_t *task) 
{
  assert(false);
}

lithe_sched_funcs_t child_funcs = {
  .enter = child_enter,
  .yield = child_yield,
  .reg = child_reg,
  .unreg = child_unreg,
  .request = child_request,
  .unblock = child_unblock,
};

void child_start()
{
  printf("child_start\n");
  lithe_sched_reenter();
}

void root_main()
{
  printf("root_main start\n");

  /* Create a start task for a child scheduler */
  lithe_task_t *task = lithe_task_create(child_start, NULL, NULL);
  /* Register a child scheduler: Blocks until scheduler finishes */
  lithe_sched_register(&child_funcs, NULL, task);
  /* Destroy the start task of the child scheduler */
  lithe_task_destroy(task);

  printf("root_main finish\n");
  /* Yield the vcore */
  /* TODO: Need to destroy this task somehow */
  /* TODO: Consider making this main task special and letting it just run to
   * completion and wrapping it inside the lithe code with a destroy call and
   * an implicit call to lithe_sched_yield() instead of the following brutal
   * hack of calling reenter directly and then yielding in there on a flag ... */
  lithe_sched_reenter();
}

void root_enter(void *__sched) 
{
  printf("root_enter\n");
  root_sched_t *sched = (root_sched_t*) __sched;

  /* Brutal hack to yield the vcore back properly.  Fixing in next revision */
  static int num_mains_created = 0;
  if(num_mains_created < sched->expected_children) {
    num_mains_created++;
    /* Create a main task on this vcore from which to launch a child scheduler */
    lithe_task_t *task = lithe_task_create(root_main, NULL, NULL);
    /* Run the task */
    lithe_task_run(task);
  }

  /* Check to see if our number of expected children has dropped to 0 */
  bool done = false;
  spinlock_lock(&sched->lock);
    if(--sched->expected_children == 0)
      done = true;
    printf("sched->expected_children: %d\n", sched->expected_children);
  spinlock_unlock(&sched->lock);

  /* If so, unblock the the root_sched start_task on this vcore */
  if(done)
    lithe_task_unblock(sched->start_task);
  else 
  /* Otherwise just yield the vcore */
    lithe_sched_yield();
}

void root_yield(void *__sched, lithe_sched_t *__child)
{ 
  assert(false);
}

void root_reg(void *__sched, lithe_sched_t *__child) 
{
  root_sched_t *sched = (root_sched_t*)__sched;
  child_sched_t *child = malloc(sizeof(child_sched_t));
  child->sched = __child;

  spinlock_lock(&sched->lock);
    LIST_INSERT_HEAD(&sched->children, child, link);
  spinlock_unlock(&sched->lock);
}

void root_unreg(void *__sched, lithe_sched_t *__child) 
{
  root_sched_t *sched = (root_sched_t*)__sched;
  child_sched_t *child;

  spinlock_lock(&sched->lock);
    LIST_FOREACH(child, &sched->children, link) {
      if(child->sched == __child);
        break;
    }
    assert(child != NULL);
    LIST_REMOVE(child, link);
  spinlock_unlock(&sched->lock);

  free(child);
}

int root_request(void *__sched, lithe_sched_t *__child, int k) 
{
  return lithe_sched_request(k);
}

void root_unblock(void *__sched, lithe_task_t *task) 
{
  root_sched_t *sched = (root_sched_t *)__sched;
  assert(task == sched->start_task);
  lithe_task_run(task);
}

lithe_sched_funcs_t root_funcs = {
  .enter = root_enter,
  .yield = root_yield,
  .reg = root_reg,
  .unreg = root_unreg,
  .request = root_request,
  .unblock = root_unblock,
};

void block_root(lithe_task_t *task, void *arg)
{
  printf("block_root\n");
  lithe_sched_reenter();
}

void root_sched_start(void *arg)
{
  printf("root_sched_start\n");
  root_sched_t *sched = lithe_sched_current();
  spinlock_lock(&sched->lock);
    sched->expected_children = lithe_sched_request(max_vcores()) + 1;
  spinlock_unlock(&sched->lock);
  lithe_task_block(block_root, NULL);
  lithe_sched_unregister();
}

int main()
{
  printf("main start\n");

  /* Instantiate a root scheduler */
  root_sched_t root_sched;
  /* Initiailize the root scheduler's lock */
  spinlock_init(&root_sched.lock);
  /* Initialize the root scheduler's list of children schedulers */
  LIST_INIT(&root_sched.children);
  /* Create the start task of the root scheduler */
  root_sched.start_task = lithe_task_create(root_sched_start, NULL, NULL);
  /* Register the root scheduler: Blocks until scheduler finishes */
  lithe_sched_register(&root_funcs, &root_sched, root_sched.start_task);
  /* Destroy the start task of the root scheduler */
  lithe_task_destroy(root_sched.start_task);
  
  printf("main finish\n");
  return 0;
}

