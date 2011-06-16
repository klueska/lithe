#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <lithe/lithe.h>
#include <ht/atomic.h>

const int COUNT = 31;
static int count = 0;

void child_enter(void *__this) 
{
  printf("child_enter\n");
  fetch_and_add((int *) &count, 1);
  lithe_sched_yield();
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

void child_request(void *__this, lithe_sched_t *child, int k) 
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

void __child(void *arg)
{
  printf("__child\n");
  lithe_sched_request(COUNT);
  while (coherent_read(count) < COUNT);
  lithe_sched_unregister();
}

void child()
{
  printf("child\n");
  lithe_task_stack_t stack = {malloc(8192), 8192};
  lithe_task_t *task = lithe_task_create(__child, NULL, &stack);
  lithe_sched_register(&child_funcs, NULL, task);
}

struct parent_sched {
  lithe_sched_t *child;
};

void parent_enter(void *__this) 
{
  lithe_sched_t *child = ((struct parent_sched *) __this)->child;
  if (child != NULL) {
    lithe_sched_enter(child);
  }
  lithe_sched_yield();
}

void parent_yield(void *__this, lithe_sched_t *child)
{ 
  lithe_sched_yield();
}

void parent_reg(void *__this, lithe_sched_t *child) 
{
  ((struct parent_sched *) __this)->child = child;
}

void parent_unreg(void *__this, lithe_sched_t *child) 
{
  ((struct parent_sched *) __this)->child = NULL;
}

void parent_request(void *__this, lithe_sched_t *child, int k) 
{
  lithe_sched_request(k);
}

void parent_unblock(void *__this, lithe_task_t *task) 
{
  assert(false);
}

lithe_sched_funcs_t parent_funcs = {
  .enter = parent_enter,
  .yield = parent_yield,
  .reg = parent_reg,
  .unreg = parent_unreg,
  .request = parent_request,
  .unblock = parent_unblock,
};

void __endparent(lithe_task_t *task, void *arg)
{
  printf("__endparent\n");
  assert(task == arg);
  free(task->uth.uc.uc_stack.ss_sp);
  lithe_task_destroy(task);
  free(task);
  lithe_sched_unregister();
}

void __beginparent(void *arg) 
{
  printf("__beginparent\n");
  lithe_task_t *beforetask, *aftertask;
  lithe_task_get(&beforetask);
  child();
  lithe_task_get(&aftertask);
  assert(beforetask == aftertask);
  lithe_task_block(__endparent, aftertask);
}

void parent()
{
  printf("parent\n");
  struct parent_sched __this = { NULL };
  lithe_task_stack_t stack = {malloc(8192), 8192};
  lithe_task_t *task = lithe_task_create(__beginparent, NULL, &stack);
  lithe_sched_register(&parent_funcs, &__this, task);
}

int main()
{
  printf("main start\n");
  int i;
  for (i = 0; i < 10; i++) {
    parent();
  }
  printf("main finish\n");
  return 0;
}

