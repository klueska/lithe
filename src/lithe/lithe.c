/*
 * Lithe implementation.
 *
 * Notes:
 *
 * Parents, children, and grandchildren:
 *
 * There is an unfortunate race with unregistering and yielding that
 * has (for now) forced us into a particular
 * implementation. Specifically, when a child scheduler unregisters
 * there might be a parent scheduler that is simultaneously trying to
 * enter. Really there are two options, (1) when a hard thread tries
 * to enter a child scheduler it can first lock the parent and look
 * through all of its schedulers for the child, or (2) a parent
 * scheduler can always enter a child scheduler provided that its
 * parent field is set correctly AND the child is still started.
 * The advantage of (1) is that we can free child schedulers when they
 * unregister (if a parent is invoking enter then the child will no
 * longer be a member of the parents children and so the pointer will
 * just be invalid). The disadvantage of (1), however, is that every
 * enter requires acquiring the spinlock and looking through the
 * schedulers! When doing experiements we found this to be very
 * costly. The advantage of (2) is that we don't need to look through
 * the children when we enter. The disadvantage of (2), however, is
 * that we need to keep every child scheduler around so that we can
 * check whether or not that child is still started! It has been
 * reported that this creates a lot of schedulers that never get
 * cleaned up. The practical impacts of this are still unknown
 * however. A hybrid solution is probably the best here. Something
 * that looks through the children without acquiring the lock (since
 * most of the time the scheduler will be found in the children), and
 * frees a child when it unregisters.
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <asm/unistd.h>

#include <ht/atomic.h>
#include <spinlock.h>

#include <sys/mman.h>
#include <sys/resource.h>

#include <lithe/lithe.h>
#include <lithe/fatal.h>

#ifndef __linux__
#error "expecting __linux__ to be defined (for now, Lithe only runs on Linux)"
#endif

/* Struct to keep track of the internal state of each of the 2nd-level
 * schedulers managed by lithe */
struct lithe_sched_idata {
  /* Lock for managing scheduler. */
  int lock;

  /* State of scheduler. */
  enum { 
    REGISTERED,
    UNREGISTERING,
    UNREGISTERED 
  } state;

  /* Number of vcores currently owned by this scheduler. */
  int vcores;

  /* Pointer to the start task of this scheduler */
  lithe_task_t *start_task;

  /* Scheduler's parent scheduler */
  lithe_sched_t *parent;

  /* Parent task from which this scheduler was started. */
  lithe_task_t *parent_task;

  /* Scheduler's children schedulers (necessary to insure calling enter is
   * valid). */
  lithe_sched_t *children;

  /* Linked list of sibling's (list head is parent->children). */ 
  lithe_sched_t *next, *prev;
};

/* Struct to hold the function pointer and argument of a function to be called
 * in vcore_entry after one of the lithe functions yields from a task */
typedef struct lithe_vcore_func {
  void (*func) (void *);
  void *arg;
} lithe_vcore_func_t;

/* Hooks from uthread code into lithe */
static uthread_t*   lithe_init(void);
static void         lithe_vcore_entry(void);
static uthread_t*   lithe_thread_create(void (*func)(void), void *udata);
static void         lithe_thread_runnable(uthread_t *uthread);
static void         lithe_thread_yield(uthread_t *uthread);
static void         lithe_thread_exit(uthread_t *uthread);
static unsigned int lithe_vcores_wanted(void);

/* Unique function pointer table required by uthread code */
struct schedule_ops lithe_sched_ops = {
  .sched_init       = lithe_init,
  .sched_entry      = lithe_vcore_entry,
  .thread_create    = lithe_thread_create,
  .thread_runnable  = lithe_thread_runnable,
  .thread_yield     = lithe_thread_yield,
  .thread_exit      = lithe_thread_exit,
  .vcores_wanted    = lithe_vcores_wanted,
  .preempt_pending  = NULL, /* lithe_preempt_pending, */
  .spawn_thread     = NULL, /* lithe_spawn_thread, */
};
/* Publish these schedule_ops, overriding the weak defaults setup in uthread */
struct schedule_ops *sched_ops __attribute__((weak)) = &lithe_sched_ops;

/* Lithe's base scheduler functions */
static lithe_sched_t *base_create();
static void base_destroy(lithe_sched_t *__this);
static void base_start(lithe_sched_t *__this);
static int base_vcore_request(lithe_sched_t *this, lithe_sched_t *child, int k);
static void base_vcore_enter(lithe_sched_t *this);
static void base_vcore_return(lithe_sched_t *this, lithe_sched_t *child);
static void base_child_started(lithe_sched_t *this, lithe_sched_t *child);
static void base_child_finished(lithe_sched_t *this, lithe_sched_t *child);
static lithe_task_t* base_task_create(lithe_sched_t *__this, void *udata);
static void base_task_yield(lithe_sched_t *__this, lithe_task_t *task);
static void base_task_exit(lithe_sched_t *__this, lithe_task_t *task);
static void base_task_runnable(lithe_sched_t *__this, lithe_task_t *task);

static const lithe_sched_funcs_t base_funcs = {
  .create          = base_create,
  .destroy         = base_destroy,
  .start           = base_start,
  .vcore_request   = base_vcore_request,
  .vcore_enter     = base_vcore_enter,
  .vcore_return    = base_vcore_return,
  .child_started   = base_child_started,
  .child_finished  = base_child_finished,
  .task_create     = base_task_create,
  .task_yield      = base_task_yield,
  .task_exit       = base_task_exit,
  .task_runnable   = base_task_runnable
};

static lithe_sched_idata_t base_idata = {
  .lock         = UNLOCKED,
  .state        = REGISTERED,
  .vcores       = 0,
  .parent       = NULL,
  .parent_task  = NULL,
  .children     = NULL,
  .next         = NULL,
  .prev         = NULL,
};

/* Base scheduler itself */
static lithe_sched_t base_sched = { 
  .funcs        = &base_funcs,
  .idata        = &base_idata
};

/* Reference to the task running the main thread. */ 
static lithe_task_t *main_task = NULL;

/* Root scheduler, i.e. the child scheduler of base. */
static lithe_sched_t *root_sched = NULL;

static __thread struct {
  /* The next task to run on this vcore when lithe_vcore_entry is called again
   * after a yield from another lithe task */
  lithe_task_t *next_task;

  /* The next function to run on this vcore when lithe_vcore_entry is called
   * after a yield from a lithe task */
  lithe_vcore_func_t *next_func;

  /* The current scheduler to run any tasks / functions with */
  lithe_sched_t *current_sched;

} lithe_tls = {NULL, NULL, NULL};
#define next_task     (lithe_tls.next_task)
#define next_func     (lithe_tls.next_func)
#define current_sched (lithe_tls.current_sched)
#define current_task  ((lithe_task_t*)current_uthread)

void vcore_ready()
{
  /* Once the vcore subsystem is up and running, initialize the uthread
   * library, which will, in turn, eventually call lithe_init() for us */
  uthread_init();
}

static uthread_t* lithe_init()
{
  /* Create a lithe task for the main thread to run in */
  main_task = (lithe_task_t*)calloc(1, sizeof(lithe_task_t));
  assert(main_task);

  /* Set the scheduler associated with the task to be the base scheduler */
  current_sched = &base_sched;

  /* Return a reference to the main task back to the uthread library. We will
   * resume this task once lithe_vcore_entry() is called from the uthread
   * library */
  return (uthread_t*)(main_task);
}

static void __attribute__((noreturn)) __lithe_sched_reenter()
{
  assert(in_vcore_context());

  /* Enter current scheduler. */
  current_sched->funcs->vcore_enter(current_sched);
  fatal("lithe: returned from enter");
}

static void __attribute__((noreturn)) lithe_vcore_entry()
{
  /* Make sure we are in vcore context */
  assert(in_vcore_context());

  /* If we are entering this vcore for the first time, we need to set the
   * current scheduler appropriately and up the vcore count for it */
  if(current_sched == NULL) {
    /* Set the current scheduler as the base scheduler */
    current_sched = &base_sched;
    /* Up our vcore count for the base scheduler */
    __sync_fetch_and_add(&base_sched.idata->vcores, 1);
  }

  /* If current_task is set, then just resume it. This will happen in one of 2
   * cases: 1) It is the first, i.e. main thread, or 2) The current vcore was
   * taken over to run a signal handler from the kernel, and is now being
   * restarted */
  if(current_task) {
    current_sched = uthread_get_tls_var(&current_task->uth, current_sched);
    run_current_uthread();
    assert(0); // Should never return from running task
  }

  /* Otherwise, if next_task is set, start it off anew. */
  if(next_task) {
    lithe_task_t *task = next_task;
    current_sched = uthread_get_tls_var(&task->uth, current_sched);
    next_task = NULL;
    run_uthread(&task->uth);
    assert(0); // Should never return from running task
  }

  /* Otherwise, if next_function is set, call it */
  if(next_func) {
    lithe_vcore_func_t *func = next_func;
    next_func = NULL;
    func->func(func->arg);
    assert(0); // Should never return from called function 
  }

  /* Otherwise, just reenter vcore_enter of whatever the current scheduler is */
  __lithe_sched_reenter();
  assert(0); // Should never return from entered scheduler
}

static int __allocate_lithe_task_stack(lithe_task_stack_t **stack)
{
  /* Set things up to create a 4 page stack */
  int pagesize = getpagesize();
  size_t size = pagesize * 4;

  /* Build the protection and flags for the mmap call to allocate the stack */
  const int protection = (PROT_READ | PROT_WRITE | PROT_EXEC);
  const int flags = (MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT);

  /* mmap the stack in */
  void *sp = mmap(NULL, size, protection, flags, -1, 0);
  if (sp == MAP_FAILED) 
    return -1;

  /* Disallow all memory access to the last page. */
  if (mprotect(sp, pagesize, PROT_NONE) != 0)
    return -1;

  /* Set the paramters properly before returning */
  (*stack)->sp = sp;
  (*stack)->size = size;
  return 0;
}

static void __free_lithe_task_stack(lithe_task_stack_t *stack)
{
  munmap(stack->sp, stack->size);
}

static uthread_t* lithe_thread_create(void (*func)(void), void *udata)
{
  return (uthread_t*)current_sched->funcs->task_create(current_sched, udata);
}

static void lithe_thread_runnable(struct uthread *uthread)
{
  assert(current_task);
  current_sched->funcs->task_runnable(current_sched, current_task);
}

static void lithe_thread_yield(struct uthread *uthread)
{
  assert(current_task);
  current_sched->funcs->task_yield(current_sched, current_task);
}

static void lithe_thread_exit(struct uthread *uthread)
{
  assert(current_task);
  current_sched->funcs->task_exit(current_sched, current_task);
}

static unsigned int lithe_vcores_wanted(void)
{
  return 0;
}


static lithe_sched_t *base_create()
{
  fatal("Trying to recreate the base scheduler!\n");
}

static void base_destroy(lithe_sched_t *__this)
{
  fatal("Trying to destroy the base scheduler!\n");
}

static void base_start(lithe_sched_t *__this)
{
  fatal("Trying to restart the base scheduler!\n");
}

static void base_vcore_enter(lithe_sched_t *__this)
{
  assert(root_sched != NULL);
  lithe_vcore_grant(root_sched);
}

static void base_vcore_return(lithe_sched_t *__this, lithe_sched_t *sched)
{
  /* Cleanup tls and yield the vcore back to the system. */
  __sync_fetch_and_add(&base_sched.idata->vcores, -1);
  memset(&lithe_tls, 0, sizeof(lithe_tls));
  vcore_yield();
}

static void base_child_started(lithe_sched_t *__this, lithe_sched_t *sched)
{
  assert(root_sched == NULL);
  root_sched = sched;
}

static void base_child_finished(lithe_sched_t *__this, lithe_sched_t *sched)
{
  assert(root_sched == sched);
  root_sched = NULL;
}

static int base_vcore_request(lithe_sched_t *__this, lithe_sched_t *sched, int k)
{
  assert(root_sched == sched);
  return vcore_request(k);
}

static lithe_task_t *base_task_create(lithe_sched_t *__this, void *udata)
{
  /* Create a new lithe task */
  lithe_task_t *t = (lithe_task_t*)calloc(1, sizeof(lithe_task_t));
  assert(t);

  /* Set up the stack for this lithe task */
  lithe_task_stack_t *stack = &(t->stack);
  assert(__allocate_lithe_task_stack(&stack) == 0);

  /* Initialize the uthread associated with the newly created task with the
   * stack */
  init_uthread_stack(&t->uth, stack->sp, stack->size); 

  /* Return the new thread */
  return t;
}

static void base_task_yield(lithe_sched_t *__this, lithe_task_t *task)
{
  // Do nothing, lust let it yield as normal...
}

static void base_task_exit(lithe_sched_t *__this, lithe_task_t *task)
{
  __free_lithe_task_stack(&task->stack);
}

static void base_task_runnable(lithe_sched_t *__this, lithe_task_t *task)
{
  fatal("Tasks created by the base scheduler should never need to be made runnable!\n");
}

void *lithe_sched_current()
{
  return current_sched;
}

int lithe_vcore_grant(lithe_sched_t *child)
{
  assert(child);
  assert(child->idata->parent);
  assert(in_vcore_context());

  lithe_sched_t *parent = current_sched;

  /* TODO: Check if child is descendant of parent? */

  /*
   * Optimistically try and join child. We need to try and increment
   * the childrens number of threads before we check the childs state
   * because that is the order of writes that an unregistering child
   * will take.
   */
  if (child->idata->state == REGISTERED) {
    /* Leave parent, join child. */
    assert(child != &base_sched);
    current_sched = child;
    __sync_fetch_and_add(&(child->idata->vcores), 1);

    if (child->idata->state != REGISTERED) {
      /* Leave child, join parent. */
      __sync_fetch_and_add(&(child->idata->vcores), -1);
      current_sched = parent;

      /* Signal unregistering hard thread. */
      if (child->idata->state == UNREGISTERING) {
        if (child->idata->vcores == 0) {
          child->idata->state = UNREGISTERED;
        }
      }
    }
  }

  /* Enter current_sched, updated or not... */
  __lithe_sched_reenter();
  fatal("lithe: returned from enter");
}

void lithe_vcore_yield()
{
  assert(in_vcore_context());
  assert(current_sched != &base_sched);

  lithe_sched_t *parent = current_sched->idata->parent;
  lithe_sched_t *child = current_sched;

  /* Leave child, join parent. */
  __sync_fetch_and_add(&(child->idata->vcores), -1);

  /* Signal unregistering hard thread. */
  if (child->idata->state == UNREGISTERING) {
    if (child->idata->vcores == 0) {
      child->idata->state = UNREGISTERED;
    }
  }

  /* Yield the vcore to the parent */
  current_sched = parent;
  parent->funcs->vcore_return(parent, child);
  __lithe_sched_reenter();
}

static void lithe_sched_finish();
static void __lithe_sched_start()
{
  /* Run the start function of the scheduler */
  assert(!in_vcore_context());
  assert(current_sched);
  current_sched->funcs->start(current_sched);
  
  /* Jump into vcore context to destroy this task and scheduler */
  lithe_vcore_func_t func = {lithe_sched_finish, NULL};
  vcore_set_tls_var(vcore_id(), next_func, &func);
  uthread_yield();
}

int lithe_sched_start(const lithe_sched_funcs_t *funcs)
{
  assert(funcs);
  assert(!in_vcore_context());
  assert(current_task);
  assert(current_sched);

  /* Set the parent to be the current scheduler */
  lithe_sched_t *parent = current_sched;

  /* Create the child scheduler */
  lithe_sched_t *child = funcs->create();
  lithe_sched_idata_t *child_idata = (lithe_sched_idata_t *) malloc(sizeof(lithe_sched_idata_t));
  assert(child);
  assert(child_idata);
  child->idata = child_idata;

  /* Check if parent scheduler is still valid. */
  spinlock_lock(&parent->idata->lock);
  { 
   if (parent->idata->state != REGISTERED) {
      spinlock_unlock(&parent->idata->lock);
      errno = EPERM;
      return -1;
    }
  }
  spinlock_unlock(&parent->idata->lock);

  /* Set-up child scheduler */
  child->funcs = funcs;
  spinlock_init(&child->idata->lock);
  child->idata->state = REGISTERED;
  child->idata->vcores = 0;
  child->idata->parent = parent;
  child->idata->parent_task = current_task;
  child->idata->children = NULL;
  child->idata->next = NULL;
  child->idata->prev = NULL;

  /* Add child to parents list of children. */
  spinlock_lock(&parent->idata->lock);
  {
    child->idata->next = parent->idata->children;
    child->idata->prev = NULL;

    if (parent->idata->children != NULL) {
      parent->idata->children->idata->prev = child;
    }

    parent->idata->children = child;
  }
  spinlock_unlock(&parent->idata->lock);

  /* Update the number of vcores now owned by this child */
  __sync_fetch_and_add(&(child->idata->vcores), 1);

  /* Inform parent. */
  parent->funcs->child_started(parent, child);

  /* Leave parent, join child. */
  current_sched = child;
  child->idata->start_task = lithe_task_create(__lithe_sched_start, NULL);
  current_sched = parent;

  vcore_set_tls_var(vcore_id(), next_task, child->idata->start_task);
  uthread_yield();
  return 0;
}

static void lithe_sched_finish()
{
  assert(in_vcore_context());

  lithe_sched_t *parent = current_sched->idata->parent;
  lithe_sched_t *child = current_sched;

  /*
   * Wait until all grandchildren have finished. The lock protects
   * against some other scheduler trying to register with child
   * (another grandchild) as well as our write to child idata->
   */
  spinlock_lock(&child->idata->lock);
  {
    lithe_sched_t *sched = child->idata->children;

    while (sched != NULL) {
      while (sched->idata->state != UNREGISTERED) {
        rdfence();
        atomic_delay();
      }
      sched = sched->idata->next;
    }

    child->idata->state = UNREGISTERING;
  }
  spinlock_unlock(&child->idata->lock);

  /* Update child's vcore count */
  __sync_fetch_and_add(&(child->idata->vcores), -1);

  if (child->idata->vcores == 0) {
    child->idata->state = UNREGISTERED;
  }

  /* Wait until all threads have left child. */
  while (child->idata->state != UNREGISTERED) {
    /* TODO: Consider 'monitor' and 'mwait' or even MCS. */
    rdfence();
    atomic_delay();
  }
  /* Inform the parent that this child scheduler has finished */
  parent->funcs->child_finished(parent, child);

  /* Free all grandchildren schedulers. */
  lithe_sched_t *sched = child->idata->children;
  while (sched != NULL) {
    lithe_sched_t *next = sched->idata->next;
    free(sched->idata);
    sched->funcs->destroy(sched);
    sched = next;
  }

  /* Return control to the parent task. */
  vcore_set_tls_var(vcore_id(), next_task, child->idata->parent_task);
  lithe_vcore_entry();
}

int lithe_vcore_request(int k)
{
  assert(current_sched);
  lithe_sched_t *parent = current_sched->idata->parent;
  lithe_sched_t *child = current_sched;

  current_sched = parent;
  int granted = parent->funcs->vcore_request(parent, child, k);
  current_sched = child;
  return granted;
}

void __lithe_task_create(void (*func) (void)) 
{
  func();
  uthread_exit();
}

lithe_task_t *lithe_task_create(void (*func) (void), void *udata) 
{
  assert(func != NULL);

  lithe_task_t *task = (lithe_task_t*)uthread_create(func, udata);
  assert(task);
  assert(task->stack.sp);

  init_uthread_stack(&task->uth, task->stack.sp, task->stack.size);
  init_uthread_entry(&task->uth, __lithe_task_create, 1, func);
  uthread_set_tls_var(&task->uth, current_sched, current_sched);
  return task;
}

lithe_task_t *lithe_task_self()
{
  return current_task;
}

int lithe_task_run(lithe_task_t *task)
{
  assert(task);
  assert(in_vcore_context());

  vcore_set_tls_var(vcore_id(), next_task, task);
  lithe_vcore_entry();
  return -1;
}

void __lithe_task_block(void *arg)
{
  assert(arg);
  assert(in_vcore_context());

  struct { 
    void (*func) (lithe_task_t *, void *); 
    lithe_task_t *task;
    void *arg;
  } *real_func = arg;

  real_func->func(real_func->task, real_func->arg);
  __lithe_sched_reenter();
}

int lithe_task_block(void (*func) (lithe_task_t *, void *), void *arg)
{
  assert(func);
  assert(!in_vcore_context());
  assert(current_task);

  struct { 
    void (*func) (lithe_task_t *, void *); 
    lithe_task_t *task;
    void *arg;
  } real_func_arg = {func, current_task, arg};
  lithe_vcore_func_t real_func = {__lithe_task_block, &real_func_arg};

  vcore_set_tls_var(vcore_id(), next_func, &real_func);
  uthread_yield();
  return 0;
}

int lithe_task_unblock(lithe_task_t *task)
{
  assert(task);

  current_sched->funcs->task_runnable(current_sched, task);
  return 0;
}

