/*
 * Lithe implementation.
 *
 * Notes:
 *
 * Trampoline Context/Stack:
 *
 * There are currently two ways to get onto the trampoline. The first
 * is using the macro swap_to_trampoline (see source for details),
 * while the second is using the ucontext routines (see
 * lithe_task_block for details). The first uses less of the
 * trampoline stack space but the second is (arguably) better style.
 *
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
 * parent field is set correctly AND the child is still registered.
 * The advantage of (1) is that we can free child schedulers when they
 * unregister (if a parent is invoking enter then the child will no
 * longer be a member of the parents children and so the pointer will
 * just be invalid). The disadvantage of (1), however, is that every
 * enter requires acquiring the spinlock and looking through the
 * schedulers! When doing experiements we found this to be very
 * costly. The advantage of (2) is that we don't need to look through
 * the children when we enter. The disadvantage of (2), however, is
 * that we need to keep every child scheduler around so that we can
 * check whether or not that child is still registered! It has been
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
#include <ucontext.h>
#include <unistd.h>

#include <asm/unistd.h>

#include <ht/atomic.h>
#include <ht/ht.h>
#include <spinlock.h>

#include <sys/mman.h>
#include <sys/resource.h>

#include "lithe.h"
#include "fatal.h"

#ifndef __linux__
#error "expecting __linux__ to be defined (for now, Lithe only runs on Linux)"
#endif


struct lithe_sched {
  /* Lock for managing scheduler. */
  int lock;

  /* State of scheduler. */
  enum { REGISTERED,
	 UNREGISTERING,
	 UNREGISTERED } state;

  /* Number of threads executing within scheduler. */
  int nthreads;

  /* Scheduler's parent. */
  lithe_sched_t *parent;

  /* Scheduler's parent task. */
  lithe_task_t *task;

  /* Scheduler's children (necessary to insure calling enter is valid). */
  lithe_sched_t *children;

  /* Linked list of sibling's (list head is parent->children). */ 
  lithe_sched_t *next, *prev;

  /* Scheduler's functions. */
  const lithe_sched_funcs_t *funcs;

  /* Scheduler's 'this' pointer. */
  void *this;
};


/* Base scheduler functions (declarations). */
void base_enter(void *this);
void base_yield(void *this, lithe_sched_t *sched);
void base_reg(void *this, lithe_sched_t *sched);
void base_unreg(void *this, lithe_sched_t *sched);
void base_request(void *this, lithe_sched_t *sched, int k);
void base_unblock(void *this, lithe_task_t *task);

static const lithe_sched_funcs_t base_funcs = {
  .enter   = base_enter,
  .yield   = base_yield,
  .reg     = base_reg,
  .unreg   = base_unreg,
  .request = base_request,
  .unblock = base_unblock,
};

/* Base scheduler. */
static lithe_sched_t base = { 
  .lock     = UNLOCKED,
  .state    = REGISTERED,
  .nthreads = 1,  /* Accounting for main (defunct thread). */
  .parent   = NULL,
  .children = NULL,
  .next     = NULL,
  .prev     = NULL,
  .funcs    = &base_funcs,
  .this   = (void *) 0xBADC0DE,
};

/* Root scheduler, i.e. the child scheduler of base. */
static lithe_sched_t *root = NULL;

/* Root scheduler's requested number of hard threads. */
//static int requested = 0;

/* Current scheduler of executing hard thread (initially set to base). */
__thread lithe_sched_t *current = &base;

/* Current task of executing hard thread. */
__thread lithe_task_t *task = NULL;

/* Task used when transitioning between schedulers. */
__thread lithe_task_t trampoline = {{0}};

#define has_trampoline() (trampoline.ctx.uc_stack.ss_sp != NULL)


#ifdef __x86_64__
# define swap_to_trampoline()						\
{									\
  task = &trampoline;							\
  void *stack = (void *)						\
    (((size_t) trampoline.ctx.uc_stack.ss_sp)				\
     + (trampoline.ctx.uc_stack.ss_size - __alignof__(long double)));	\
  asm volatile ("mov %0, %%rsp" : : "r" (stack));			\
}
#elif __i386__
# define swap_to_trampoline()						\
{									\
  task = &trampoline;							\
  void *stack = (void *)						\
    (((size_t) trampoline.ctx.uc_stack.ss_sp)				\
     + (trampoline.ctx.uc_stack.ss_size - __alignof__(long double)));	\
  asm volatile ("mov %0, %%esp" : : "r" (stack));			\
}
#else
# error "missing implementations for your architecture"
#endif




/* TODO(benh): Implement this assuming no stack! */
#define setup_trampoline()                                                   \
{                                                                            \
  /* Determine trampoline stack size. */                                     \
  int pagesize = getpagesize();                                              \
                                                                             \
  size_t size = pagesize * 4;                                                \
                                                                             \
  const int protection = (PROT_READ | PROT_WRITE);                           \
  const int flags = (MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT);               \
                                                                             \
  void *stack = mmap(NULL, size, protection, flags, -1, 0);                  \
                                                                             \
  if (stack == MAP_FAILED) {                                                 \
    fatalerror("lithe: could not allocate trampoline");                      \
  }                                                                          \
                                                                             \
  /* Disallow all memory access to the last page. */                         \
  if (mprotect(stack, pagesize, PROT_NONE) != 0) {                           \
    fatalerror("lithe: could not protect page");                             \
  }                                                                          \
                                                                             \
  /* Prepare context. */                                                     \
  if (getcontext(&trampoline.ctx)) {                                         \
    fatalerror("lithe: could not get context");                              \
  }                                                                          \
                                                                             \
  trampoline.ctx.uc_stack.ss_sp = stack;                                     \
  trampoline.ctx.uc_stack.ss_size = size;                                    \
  trampoline.ctx.uc_link = 0;                                                \
}


#define cleanup_trampoline()                                                 \
{                                                                            \
  /* TODO(benh): This is Linux specific, should be in sysdeps/ */            \
  void *stack = trampoline.ctx.uc_stack.ss_sp;                               \
  size_t size = trampoline.ctx.uc_stack.ss_size;                             \
                                                                             \
  long result;                                                               \
  asm volatile ("syscall"                                                    \
		: "=a" (result)                                              \
		: "0" (__NR_munmap),                                         \
                  "D" ((long) stack),                                        \
		  "S" ((long) size)                                          \
		: "r11", "rcx", "memory");                                   \
                                                                             \
  /* TODO(benh): Compiler might use the stack for conditional! */            \
  if ((unsigned long) (result) >= (unsigned long) (-127)) {                  \
    errno = -(result);                                                       \
    fatalerror("lithe: could not free trampoline");                          \
  }                                                                          \
                                                                             \
  trampoline.ctx.uc_stack.ss_sp = NULL;                                      \
  trampoline.ctx.uc_stack.ss_size = 0;                                       \
}


void base_enter(void *this)
{
  lithe_sched_t *sched = root;
  if (sched != NULL)
    lithe_sched_enter(sched);

  /* TODO(benh): Don't cheat with trampolines! */
  /* Cleanup trampoline and yield hard thread. */
/*   cleanup_trampoline(); */

  /* Leave base, join hard thread world. */
  __sync_fetch_and_sub(&base.nthreads, 1);

  ht_yield();
}


void base_yield(void *this, lithe_sched_t *sched)
{
  /* TODO(benh): Don't cheat with trampolines! */
  /* Cleanup trampoline and yield hard thread. */
/*   cleanup_trampoline(); */

  /* Leave base, join hard thread world. */
  __sync_fetch_and_sub(&base.nthreads, 1);

  ht_yield();
}


void base_reg(void *this, lithe_sched_t *sched)
{
  assert(root == NULL);
  root = sched;
}


void base_unreg(void *this, lithe_sched_t *sched)
{
  assert(root == sched);
  root = NULL;
}


void base_request(void *this, lithe_sched_t *sched, int k)
{
  assert(root == sched);
  ht_request_async(k);
}


void base_unblock(void *this, lithe_task_t *task)
{
  fatal("unimplemented");
}


int lithe_initialize()
{
  printf("lithe_initialize is deprecated ...\n");
  return 0;
}


void * lithe_sched_this()
{
  return current->this;
}


int __lithe_sched_enter(lithe_sched_t *child)
{
  assert(task == &trampoline);

  lithe_sched_t *parent = current;

  if (child == NULL)
    fatal("lithe: cannot enter NULL child");

  if (child->parent != parent)
    fatal("lithe: cannot enter child; child is not descendant of parent");

  /* TODO(benh): Check if child is descendant of parent? */

  /*
   * Optimistically try and join child. We need to try and increment
   * the childrens number of threads before we check the childs state
   * because that is the order of writes that an unregistering child
   * will take.
   */
  if (child->state == REGISTERED) {
    /* Leave parent, join child. */
    assert(child != &base);
    current = child;
    __sync_fetch_and_add(&child->nthreads, 1);

    if (child->state != REGISTERED) {
      /* Leave child, join parent. */
      __sync_fetch_and_add(&child->nthreads, -1);
      current = parent;

      /* Signal unregistering hard thread. */
      if (child->state == UNREGISTERING) {
	if (child->nthreads == 0) {
	  child->state = UNREGISTERED;
	}
      }

      /* Stay in parent. */
      parent->funcs->yield(parent->this, child);
      fatal("lithe: returned from yield");
    }
  } else {
    /* Stay in parent. */
    parent->funcs->yield(parent->this, child);
    fatal("lithe: returned from yield");
  }

  /* Enter child. */
  child->funcs->enter(child->this);
  fatal("lithe: returned from enter");
}


int __lithe_sched_yield()
{
  assert(task == &trampoline);

  lithe_sched_t *parent = current->parent;
  lithe_sched_t *child = current;

  /* Leave child, join parent. */
  __sync_fetch_and_add(&child->nthreads, -1);
  current = parent;

  /* Signal unregistering hard thread. */
  if (child->state == UNREGISTERING) {
    if (child->nthreads == 0) {
      child->state = UNREGISTERED;
    }
  }

  /* Yield to parent. */
  parent->funcs->yield(parent->this, child);
  fatal("lithe: returned from yield");
}


int __lithe_sched_reenter()
{
  assert(task == &trampoline);

  /* Enter current. */
  current->funcs->enter(current->this);
  fatal("lithe: returned from enter");
}


void __lithe_sched_register(int func0, int func1, int arg0, int arg1)
{
  /* Unpackage the arguments. */
#ifdef __x86_64__
  assert(sizeof(int) != sizeof(void *));
  void (*func) (void *) = (void (*) (void *))
    (((unsigned long) func1 << 32) + (unsigned int) func0);
  void *arg = (void *)
    (((unsigned long) arg1 << 32) + (unsigned int) arg0);
#elif __i386__
  assert(sizeof(int) == sizeof(void *));
  void (*func) (void *) = (void (*) (void *)) func0;
  void *arg = (void *) arg0;
#else
# error "missing implementations for your architecture"
#endif /* __x86_64__ */

  func(arg);

  fatal("lithe: returned from register");
}


int lithe_sched_register(const lithe_sched_funcs_t *funcs,
			 void *this,
			 void (*func) (void *),
			 void *arg)
{
  if (funcs == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (func == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* We should never be on the trampoline. */
  if (task == &trampoline) {
    errno = EPERM;
    return -1;
  }

  /* 
   * N.B. In some instances, Lithe code may get called from a non-hard
   * thread (i.e., doing a pthread_create).  The problem with allowing
   * something like this is dealing with multiple threads trying to
   * simultaneosly change the root scheduler (the child scheduler of
   * base).  Really we would need one base scheduler for each thread.
   */
  if (task == NULL) {
    errno = EPERM;
    return -1;
  }

  lithe_sched_t *parent = current;
  lithe_sched_t *child = (lithe_sched_t *) malloc(sizeof(lithe_sched_t));

  if (child == NULL) {
    errno = ENOMEM;
    return -1;
  }

  /* Check if parent scheduler is still valid. */
  spinlock_lock(&parent->lock);
  {
    if (parent->state != REGISTERED) {
      spinlock_unlock(&parent->lock);
      errno = EPERM;
      return -1;
    }
  }
  spinlock_unlock(&parent->lock);

  /* Set-up child scheduler. */
  spinlock_init(&child->lock);
  child->state = REGISTERED;
  child->nthreads = 0;
  child->parent = parent;
  child->task = task;
  child->children = NULL;
  child->next = NULL;
  child->prev = NULL;
  child->funcs = funcs;
  child->this = this;

  /* Add child to parents list of children. */
  spinlock_lock(&parent->lock);
  {
    child->next = parent->children;
    child->prev = NULL;

    if (parent->children != NULL) {
      parent->children->prev = child;
    }

    parent->children = child;
  }
  spinlock_unlock(&parent->lock);

  /* Inform parent. */
  parent->funcs->reg(parent->this, child);

  /* Leave parent, join child. */
  current = child;
  __sync_fetch_and_add(&child->nthreads, 1);

  /* Package the arguments. */
#ifdef __x86_64__
  assert(sizeof(int) != sizeof(void *));
  int func0 = (unsigned int) (unsigned long) func;
  int func1 = (unsigned long) func >> 32;
  int arg0 = (unsigned int) (unsigned long) arg;
  int arg1 = (unsigned long) arg >> 32;
#elif __i386__
  assert(sizeof(int) == sizeof(void *));
  int func0 = (unsigned int) func;
  int func1 = 0;
  int arg0 = (unsigned int) arg;
  int arg1 = 0;
#else
# error "missing implementations for your architecture"
#endif /* __x86_64__ */

  makecontext(&trampoline.ctx, (void (*) ()) __lithe_sched_register,
	      4, func0, func1, arg0, arg1);

  task = &trampoline;
  swapcontext(&child->task->ctx, &trampoline.ctx);

  return 0;
}



int lithe_sched_register_task(const lithe_sched_funcs_t *funcs,
			      void *this,
			      lithe_task_t *_task)
{
  if (funcs == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (_task == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* We should never be on the trampoline. */
  if (task == &trampoline) {
    errno = EPERM;
    return -1;
  }

  /* 
   * N.B. In some instances, Lithe code may get called from a non-hard
   * thread (i.e., doing a pthread_create).  The problem with allowing
   * something like this is dealing with multiple threads trying to
   * simultaneosly change the root scheduler (the child scheduler of
   * base).  Really we would need one base scheduler for each thread.
   */
  if (task == NULL) {
    errno = EPERM;
    return -1;
  }

  lithe_sched_t *parent = current;
  lithe_sched_t *child = (lithe_sched_t *) malloc(sizeof(lithe_sched_t));

  if (child == NULL) {
    errno = ENOMEM;
    return -1;
  }

  /* Check if parent scheduler is still valid. */
  spinlock_lock(&parent->lock);
  { 
   if (parent->state != REGISTERED) {
      spinlock_unlock(&parent->lock);
      errno = EPERM;
      return -1;
    }
  }
  spinlock_unlock(&parent->lock);

  /* Set-up child scheduler. */
  spinlock_init(&child->lock);
  child->state = REGISTERED;
  child->nthreads = 0;
  child->parent = parent;
  child->task = task;
  child->children = NULL;
  child->next = NULL;
  child->prev = NULL;
  child->funcs = funcs;
  child->this = this;

  /* Add child to parents list of children. */
  spinlock_lock(&parent->lock);
  {
    child->next = parent->children;
    child->prev = NULL;

    if (parent->children != NULL) {
      parent->children->prev = child;
    }

    parent->children = child;
  }
  spinlock_unlock(&parent->lock);

  /* Inform parent. */
  parent->funcs->reg(parent->this, child);

  /* Leave parent, join child. */
  current = child;
  __sync_fetch_and_add(&child->nthreads, 1);

  /* Initialize task. */
  if (getcontext(&_task->ctx) < 0) {
    fatalerror("lithe: could not get context");
  }

  /* TODO(benh): The 'sched' field is only difference from destroyed task. */
  _task->ctx.uc_stack.ss_sp = 0;
  _task->ctx.uc_stack.ss_size = 0;
  _task->ctx.uc_link = 0;

  _task->sched = current;

  _task->tls = NULL;

  task = _task;

  return 0;
}


int lithe_sched_unregister()
{
  lithe_sched_t *parent = current->parent;
  lithe_sched_t *child = current;

  /*
   * Wait until all grandchildren have unregistered. The lock protects
   * against some other scheduler trying to register with child
   * (another grandchild) as well as our write to child state.
   */
  spinlock_lock(&child->lock);
  {
    lithe_sched_t *sched = child->children;

    while (sched != NULL) {
      while (sched->state != UNREGISTERED) {
	rdfence();
	atomic_delay();
      }
      sched = sched->next;
    }

    child->state = UNREGISTERING;
  }
  spinlock_unlock(&child->lock);

  /* Leave child, join parent. */
  __sync_fetch_and_add(&child->nthreads, -1);
  current = parent;

  if (child->nthreads == 0) {
    child->state = UNREGISTERED;
  }

  /* Update task. */
  task = child->task;

  /* Inform parent. */
  parent->funcs->unreg(parent->this, child);

  /* Wait until all threads have left child. */
  while (child->state != UNREGISTERED) {
    /* TODO(benh): Consider 'monitor' and 'mwait' or even MCS. */
    rdfence();
    atomic_delay();
  }

  /* Free grandchildren schedulers. */
  lithe_sched_t *sched = child->children;

  while (sched != NULL) {
    lithe_sched_t *next = sched->next;
    free(sched);
    sched = next;
  }

  /* Return control. */
  setcontext(&task->ctx);

  return -1;
}


int lithe_sched_unregister_task(lithe_task_t **_task)
{
  if (_task == NULL) {
    errno = EINVAL;
    return -1;
  }

  lithe_sched_t *parent = current->parent;
  lithe_sched_t *child = current;

  /* Wait until all grandchildren have unregistered. */
  /* TODO(*): Does it make more sense to inform the parent first? */
  spinlock_lock(&child->lock);
  {
    lithe_sched_t *sched = child->children;

    while (sched != NULL) {
      while (sched->state != UNREGISTERED) {
	rdfence();
	atomic_delay();
      }
      sched = sched->next;
    }

    child->state = UNREGISTERING;
  }
  spinlock_unlock(&child->lock);

  /* Leave child, join parent. */
  __sync_fetch_and_add(&child->nthreads, -1);
  current = parent;

  if (child->nthreads == 0) {
    child->state = UNREGISTERED;
  }

  /* Update task. */
  *_task = task;
  task = child->task;

  /* Inform parent. */
  parent->funcs->unreg(parent->this, child);

  /* Wait until all threads have left child. */
  while (child->state != UNREGISTERED) {
    /* TODO(benh): Consider 'monitor' and 'mwait' or even MCS. */
    rdfence();
    atomic_delay();
  }

  /* Free grandchildren schedulers. */
  lithe_sched_t *sched = child->children;

  while (sched != NULL) {
    lithe_sched_t *next = sched->next;
    free(sched);
    sched = next;
  }

  /* Return control. */
  return 0;
}


int lithe_sched_request(int k)
{
  lithe_sched_t *parent = current->parent;
  lithe_sched_t *child = current;

  /* Explicitly use NULL task. */
  lithe_task_t *__task = task;
  task = NULL;

  current = parent;
  parent->funcs->request(parent->this, child, k);
  current = child;

  task = __task;

  return 0;
}


int lithe_task_init(lithe_task_t *_task, stack_t *stack)
{
  if (_task == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (stack == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (getcontext(&_task->ctx) < 0) {
    fatalerror("lithe: could not get context");
  }

  _task->ctx.uc_stack.ss_sp = stack->ss_sp;
  _task->ctx.uc_stack.ss_size = stack->ss_size;
  _task->ctx.uc_link = 0;

  _task->sched = current;

  _task->tls = NULL;

  return 0;
}


int lithe_task_destroy(lithe_task_t *_task)
{
  if (_task == NULL) {
    errno = EINVAL;
    return -1;
  }

  memset(_task, 0, sizeof(lithe_task_t));

  return 0;
}


int lithe_task_get(lithe_task_t **_task)
{
  if (_task == NULL) {
    errno = EINVAL;
    return -1;
  }

  *_task = task;

  return 0;
}


int lithe_task_settls(void *tls) 
{
  if ((task == &trampoline) || (task == NULL)) {
    errno = EPERM;
    return -1;
  }
  
  task->tls = tls;

  return 0;
}


int lithe_task_gettls(void **tls) 
{
  if ((task == &trampoline) || (task == NULL)) {
    errno = EPERM;
    return -1;
  }

  if (tls == NULL) {
    errno = EINVAL;
    return -1;
  }
  
  *tls = task->tls;

  return 0;
}


void __lithe_task_do(int func0, int func1, int arg0, int arg1)
{
  /* Unpackage the arguments. */
#ifdef __x86_64__
  assert(sizeof(int) != sizeof(void *));
  void (*func) (void *) = (void (*) (void *))
    (((unsigned long) func1 << 32) + (unsigned int) func0);
  void *arg = (void *)
    (((unsigned long) arg1 << 32) + (unsigned int) arg0);
#elif __i386__
  assert(sizeof(int) == sizeof(void *));
  void (*func) (void *) = (void (*) (void *)) func0;
  void *arg = (void *) arg0;
#else
# error "missing implementations for your architecture"
#endif /* __x86_64__ */

  func(arg);

  fatal("lithe: returned from executing task");
}


int lithe_task_do(lithe_task_t *_task, void (*func) (void *), void *arg)
{
  if (_task == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (_task->sched == NULL) {
    errno = ESRCH;
    return -1;
  }

  /* TODO(benh): Must we be on the trampoline? */
  if (task != &trampoline) {
    errno = EPERM;
    return -1;
  }

  /* Package the arguments. */
#ifdef __x86_64__
  assert(sizeof(int) != sizeof(void *));
  int func0 = (unsigned int) (unsigned long) func;
  int func1 = (unsigned long) func >> 32;
  int arg0 = (unsigned int) (unsigned long) arg;
  int arg1 = (unsigned long) arg >> 32;
#elif __i386__
  assert(sizeof(int) == sizeof(void *));
  int func0 = (unsigned int) func;
  int func1 = 0;
  int arg0 = (unsigned int) arg;
  int arg1 = 0;
#else
# error "missing implementations for your architecture"
#endif /* __x86_64__ */

  makecontext(&_task->ctx, (void (*) ()) __lithe_task_do,
	      4, func0, func1, arg0, arg1);

  task = _task;
  setcontext(&_task->ctx);
  return -1;
}


void __lithe_task_block(int func0, int func1,
			int task0, int task1,
			int arg0, int arg1)
{
  /* Unpackage the arguments. */
#ifdef __x86_64__
  assert(sizeof(int) != sizeof(void *));
  void (*func) (lithe_task_t *, void *) = (void (*) (lithe_task_t *, void *))
    (((unsigned long) func1 << 32) + (unsigned int) func0);
  lithe_task_t *__task = (lithe_task_t *)
    (((unsigned long) task1 << 32) + (unsigned int) task0);
  void *arg = (void *)
    (((unsigned long) arg1 << 32) + (unsigned int) arg0);
#elif __i386__
  void (*func) (lithe_task_t *, void *) =
    (void (*) (lithe_task_t *, void *)) func0;
  lithe_task_t *__task = (lithe_task_t *) task0;
  void *arg = (void *) arg0;
#else
# error "missing implementations for your architecture"
#endif /* __x86_64__ */

  func(__task, arg);

  fatal("lithe: returned from executing task");
}


int lithe_task_block(void (*func) (lithe_task_t *, void *), void *arg)
{
  if (func == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* Confirm we aren't blocking using trampoline. */
  if (task == &trampoline) {
    errno = EPERM;
    return -1;
  }

  /* Confirm there is a task to block (i.e. no task during 'request'). */
  if (task == NULL) {
    errno = EPERM;
    return -1;
  }

  /* Package the arguments. */
#ifdef __x86_64__
  assert(sizeof(int) != sizeof(void *));
  int func0 = (unsigned int) (unsigned long) func;
  int func1 = (unsigned long) func >> 32;
  int task0 = (unsigned int) (unsigned long) task;
  int task1 = (unsigned long) task >> 32;
  int arg0 = (unsigned int) (unsigned long) arg;
  int arg1 = (unsigned long) arg >> 32;
#elif __i386__
  assert(sizeof(int) == sizeof(void *));
  int func0 = (unsigned int) func;
  int func1 = 0;
  int task0 = (unsigned int) task;
  int task1 = 0;
  int arg0 = (unsigned int) arg;
  int arg1 = 0;
#else
# error "missing implementations for your architecture"
#endif /* __x86_64__ */

  makecontext(&trampoline.ctx, (void (*) ()) __lithe_task_block,
	      6, func0, func1, task0, task1, arg0, arg1);

  lithe_task_t *__task = task;

  task = &trampoline;
  swapcontext(&__task->ctx, &trampoline.ctx);

  return 0;
}


int lithe_task_unblock(lithe_task_t *_task)
{
  if (_task == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* Explicitly use NULL task. */
  lithe_task_t *__task = task;
  task = NULL;

  lithe_sched_t *__current = current;
  current = _task->sched;
  current->funcs->unblock(current->this, _task);
  current = __current;

  task = __task;

  return 0;
}


int lithe_task_resume(lithe_task_t *_task)
{
  if (_task == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* TODO(benh): Must we be on the trampoline? */
  if (task != &trampoline) {
    errno = EPERM;
    return -1;
  }

  task = _task;
  setcontext(&_task->ctx);
  return -1;
}


void entry()
{
  /* TODO(benh): Don't cheat with trampolines (i.e. make this be assembly)! */
  if (!has_trampoline())
    setup_trampoline();

  swap_to_trampoline();

  /* Leave hard thread world, join base. */
  __sync_fetch_and_add(&(base.nthreads), 1);
  
  /* Enter base. */
  base.funcs->enter(base.this);
  fatal("lithe: returned from enter");
}


static void __attribute__((constructor)) lithe_init()
{
  /* Setup trampoline for (defunct) thread (including main thread). */
  if (!has_trampoline())
    setup_trampoline();

  /* Setup task for (defunct) thread (including main thread). */
  if (task == NULL) {
    task = (lithe_task_t *) malloc(sizeof(lithe_task_t));

    /* We have to manually initialize since we don't have a stack. */
    task->sched = &base;
    task->tls = NULL;
  }
}
