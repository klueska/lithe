/*
 * Lithe implementation.
 *
 * Notes:
 *
 * Trampoline Thread/Stack:
 *
 * The trampoline thread is created in vcore context and thus shares
 * the same tls as the vcore it is executed on.  To jump to the trampoline
 * simply call run_uthread() passing the trampoline as the parameter.
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
#include <spinlock.h>

#include <sys/mman.h>
#include <sys/resource.h>

#include <lithe/lithe.h>
#include <lithe/fatal.h>
#include <lithe/arch.h>

#ifndef __linux__
#error "expecting __linux__ to be defined (for now, Lithe only runs on Linux)"
#endif

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

/* Struct to keep track of each of the 2nd-level schedulers managed by lithe */
struct lithe_sched {
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

/* Lithe's base scheduler functions */
static void base_enter(void *this);
static void base_yield(void *this, lithe_sched_t *sched);
static void base_reg(void *this, lithe_sched_t *sched);
static void base_unreg(void *this, lithe_sched_t *sched);
static void base_request(void *this, lithe_sched_t *sched, int k);
static void base_unblock(void *this, lithe_task_t *task);

static const lithe_sched_funcs_t base_funcs = {
  .enter   = base_enter,
  .yield   = base_yield,
  .reg     = base_reg,
  .unreg   = base_unreg,
  .request = base_request,
  .unblock = base_unblock,
};

/* Base scheduler itself */
static lithe_sched_t base = { 
  .lock     = UNLOCKED,
  .state    = REGISTERED,
  .vcores   = 0,
  .parent   = NULL,
  .children = NULL,
  .next     = NULL,
  .prev     = NULL,
  .funcs    = &base_funcs,
  .this     = (void *) 0xBADC0DE,
};

/* Reference to the task running the main thread.  Whenever we create a new
 * trampoline task, use this task as our parent, even if creating from vcore
 * context */
static lithe_task_t *main_task = NULL;

/* Root scheduler, i.e. the child scheduler of base. */
static lithe_sched_t *root = NULL;

static __thread struct {
  /* Current scheduler of executing vcore (initially set to base). */
  lithe_sched_t *current_sched;
  
  /* Current task of executing vcore */
  lithe_task_t *current_task;
  
  /* Task used when transitioning between schedulers. */
  lithe_task_t *trampoline;

} lithe_tls = {NULL, NULL, NULL};
#define current_sched (lithe_tls.current_sched)
#define current_task  (lithe_tls.current_task)
#define trampoline    (lithe_tls.trampoline)

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

  /* Associate the base scheduler with this main task */
  main_task->sched = &base;

  /* Set the current task to be the main task */
  current_task = main_task;

  /* Set the current scheduler to be the base scheduler */
  current_sched = &base;

  /* Up the initial vcore count for the base scheduler for this vcore */
  __sync_fetch_and_add(&current_sched->vcores, 1);

  /* Return a reference to the main task back to the uthread library. We will
   * resume this task once lithe_vcore_entry() is called from the uthread
   * library */
  return (uthread_t*)(main_task);
}

static void __attribute__((noinline, noreturn)) __trampoline_entry()
{
  assert(current_task == trampoline);

  /* Don't treat the trampoline as a normal, schedulable uthread */
  current_uthread = NULL;

  /* Enter the scheduler. */
  current_sched->funcs->enter(current_sched->this);

  fatal("lithe: returned from enter");
}

static void __attribute__((noreturn)) lithe_vcore_entry()
{
  /* Make sure we are in vcore context */
  assert(in_vcore_context());

  /* If this vcore doesn't currently have a trampoline setup to run
   * its scheduler code, set one up */
  if(trampoline == NULL)
    trampoline = (lithe_task_t*)uthread_create(__trampoline_entry, NULL);
  assert(trampoline);

  /* If the thread local variable 'current_uthread' is set, then just resume
   * it.  This will happen in one of 2 cases: 1) It is the first, i.e. main
   * thread, or 2) The current vcore was taken over to run a signal handler
   * from the kernel, and is now being restarted */
  if(current_uthread) {
    current_task = uthread_get_tls_var(current_uthread, current_task);
    current_sched = current_task->sched;
    uthread_set_tls_var(current_uthread, trampoline, trampoline);
    run_current_uthread();
  }

  /* Otherwise, this is a newly allocated vcore, so we set ourselves up to
   * enter the base scheduler on the trampoline, and up our vcore count for
   * this scheduler */
  current_task = trampoline;
  current_task->sched = &base;
  current_sched = &base;
  __sync_fetch_and_add(&current_sched->vcores, 1);

  /* Finally, jump to the trampoline */
  run_uthread((uthread_t*)trampoline);
}

static uthread_t* lithe_thread_create(void (*func)(void), void *udata)
{
  /* Create a new lithe task */
  lithe_task_t *t = (lithe_task_t*)calloc(1, sizeof(lithe_task_t));
  assert(t);

  /* Determine the stack size. */
  int pagesize = getpagesize();
  size_t size = pagesize * 4;

  /* Build the protection and flags for the mmap call to allocate the new
   * thread's stack */
  const int protection = (PROT_READ | PROT_WRITE | PROT_EXEC);
  const int flags = (MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT);

  /* mmap the stack in */
  void *stack = mmap(NULL, size, protection, flags, -1, 0);
  if (stack == MAP_FAILED) {
    fatalerror("lithe: could not allocate trampoline");
  }

  /* Disallow all memory access to the last page. */
  if (mprotect(stack, pagesize, PROT_NONE) != 0) {
    fatalerror("lithe: could not protect page");
  }

  /* Initialize the user context with the newly created stack */
  init_user_context_stack(&t->uth.uc, stack, size); 
  /* Initialize the start function for the new thread */
  if(func != NULL)
    make_user_context(&t->uth.uc, func, 0);

  /* Return the new thread */
  return (struct uthread*)t;
}

static void lithe_thread_runnable(struct uthread *uthread)
{
}

static void lithe_thread_yield(struct uthread *uthread)
{
}

static void lithe_thread_exit(struct uthread *uthread)
{
}

static unsigned int lithe_vcores_wanted(void)
{
  return 0;
}

void base_enter(void *this)
{
  lithe_sched_t *sched = root;
  if(sched != NULL)
    lithe_sched_enter(sched);

  /* Leave base, yield back the vcore */
  base_yield(NULL, NULL);
}

void base_yield(void *this, lithe_sched_t *sched)
{
  /* Cleanup trampoline and yield the vcore. */
//  cleanup_trampoline();

  /* Leave base, yield back the vcore */
  __sync_fetch_and_sub(&(base.vcores), 1);
  vcore_yield();
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
  vcore_request(k);
}

void base_unblock(void *this, lithe_task_t *task)
{
  fatal("unimplemented");
}

void * lithe_sched_this()
{
  return current_sched->this;
}

int lithe_sched_enter(lithe_sched_t *child)
{
  assert(current_task == trampoline);

  lithe_sched_t *parent = current_sched;

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
    current_sched = child;
    __sync_fetch_and_add(&(child->vcores), 1);

    if (child->state != REGISTERED) {
      /* Leave child, join parent. */
      __sync_fetch_and_add(&(child->vcores), -1);
      current_sched = parent;

      /* Signal unregistering hard thread. */
      if (child->state == UNREGISTERING) {
	if (child->vcores == 0) {
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

int lithe_sched_yield()
{
  assert(current_task == trampoline);

  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

  /* Leave child, join parent. */
  __sync_fetch_and_add(&(child->vcores), -1);
  current_sched = parent;

  /* Signal unregistering hard thread. */
  if (child->state == UNREGISTERING) {
    if (child->vcores == 0) {
      child->state = UNREGISTERED;
    }
  }

  /* Yield to parent. */
  parent->funcs->yield(parent->this, child);
  fatal("lithe: returned from yield");
}

int lithe_sched_reenter()
{
  assert(current_task == trampoline);

  /* Enter current scheduler. */
  current_sched->funcs->enter(current_sched->this);
  fatal("lithe: returned from enter");
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
  if (current_task == trampoline) {
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
  if (current_task == NULL) {
    errno = EPERM;
    return -1;
  }

  lithe_sched_t *parent = current_sched;
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
  child->vcores = 0;
  child->parent = parent;
  child->task = current_task;
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
  current_sched = child;
  __sync_fetch_and_add(&(child->vcores), 1);

  assert(trampoline);
  make_user_context(&trampoline->uth.uc, func, 1, arg);

//  bool swap = true;
//  ucontext_t uc;
//  getcontext(&uc);
//  wrfence();
//  if(swap) {
//    swap = false;
//    memcpy(&child->task->uth.uc, &uc, sizeof(ucontext_t));
//    setcontext(&trampoline->uth.uc);
//    //run_uthread((uthread_t*)trampoline);
//  }
//  run_uthread((uthread_t*)trampoline);
//  setcontext(&trampoline->uth.uc);
  swapcontext(&child->task->uth.uc, &trampoline->uth.uc);
  return 0;
}

int lithe_sched_register_task(const lithe_sched_funcs_t *funcs,
			      void *this,
			      lithe_task_t *task)
{
  if (funcs == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (task == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* We should never be on the trampoline. */
  if (current_task == trampoline) {
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
  if (current_task == NULL) {
    errno = EPERM;
    return -1;
  }

  lithe_sched_t *parent = current_sched;
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
  child->vcores = 0;
  child->parent = parent;
  child->task = current_task;
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
  current_sched = child;
  __sync_fetch_and_add(&(child->vcores), 1);

  /* Initialize task. */
  if (getcontext(&task->uth.uc) < 0) {
    fatalerror("lithe: could not get context");
  }

  /* TODO(benh): The 'sched' field is only difference from destroyed task. */
  task->uth.uc.uc_stack.ss_sp = 0;
  task->uth.uc.uc_stack.ss_size = 0;
  task->uth.uc.uc_link = 0;

  task->sched = current_sched;

  current_task = task;

  return 0;
}

int lithe_sched_unregister()
{
  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

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
  __sync_fetch_and_add(&(child->vcores), -1);
  current_sched = parent;

  if (child->vcores == 0) {
    child->state = UNREGISTERED;
  }

  /* Update task. */
  current_task = child->task;

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
  setcontext(&current_task->uth.uc);

  return -1;
}

int lithe_sched_unregister_task(lithe_task_t **task)
{
  if (task == NULL) {
    errno = EINVAL;
    return -1;
  }

  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

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
  __sync_fetch_and_add(&(child->vcores), -1);
  current_sched = parent;

  if (child->vcores == 0) {
    child->state = UNREGISTERED;
  }

  /* Update task. */
  *task = current_task;
  current_task = child->task;

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
  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

  /* Explicitly use NULL task. */
  lithe_task_t *t = current_task;
  current_task = NULL;

  current_sched = parent;
  parent->funcs->request(parent->this, child, k);
  current_sched = child;

  current_task = t;

  return 0;
}

int lithe_task_init(lithe_task_t *task, stack_t *stack)
{
  if (task == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (stack == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (getcontext(&task->uth.uc) < 0) {
    fatalerror("lithe: could not get context");
  }

  task->uth.uc.uc_stack.ss_sp = stack->ss_sp;
  task->uth.uc.uc_stack.ss_size = stack->ss_size;
  task->uth.uc.uc_link = 0;

  task->sched = current_sched;

  return 0;
}

int lithe_task_destroy(lithe_task_t *task)
{
  if (task == NULL) {
    errno = EINVAL;
    return -1;
  }

  memset(task, 0, sizeof(lithe_task_t));

  return 0;
}

int lithe_task_get(lithe_task_t **task)
{
  if (task == NULL) {
    errno = EINVAL;
    return -1;
  }

  *task = current_task;

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
  if (current_task != trampoline) {
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

  make_user_context(&_task->uth.uc, 
                    (void (*) ()) __lithe_task_do,
	                4, func0, func1, arg0, arg1);

  current_task = _task;
  setcontext(&_task->uth.uc);
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
  lithe_task_t *task = (lithe_task_t *)
    (((unsigned long) task1 << 32) + (unsigned int) task0);
  void *arg = (void *)
    (((unsigned long) arg1 << 32) + (unsigned int) arg0);
#elif __i386__
  void (*func) (lithe_task_t *, void *) =
    (void (*) (lithe_task_t *, void *)) func0;
  lithe_task_t *task = (lithe_task_t *) task0;
  void *arg = (void *) arg0;
#else
# error "missing implementations for your architecture"
#endif /* __x86_64__ */

  func(task, arg);

  fatal("lithe: returned from executing task");
}

int lithe_task_block(void (*func) (lithe_task_t *, void *), void *arg)
{
  if (func == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* Confirm we aren't blocking using trampoline. */
  if (current_task == trampoline) {
    errno = EPERM;
    return -1;
  }

  /* Confirm there is a task to block (i.e. no task during 'request'). */
  if (current_task == NULL) {
    errno = EPERM;
    return -1;
  }

  /* Package the arguments. */
#ifdef __x86_64__
  assert(sizeof(int) != sizeof(void *));
  int func0 = (unsigned int) (unsigned long) func;
  int func1 = (unsigned long) func >> 32;
  int task0 = (unsigned int) (unsigned long) current_task;
  int task1 = (unsigned long) task >> 32;
  int arg0 = (unsigned int) (unsigned long) arg;
  int arg1 = (unsigned long) arg >> 32;
#elif __i386__
  assert(sizeof(int) == sizeof(void *));
  int func0 = (unsigned int) func;
  int func1 = 0;
  int task0 = (unsigned int) current_task;
  int task1 = 0;
  int arg0 = (unsigned int) arg;
  int arg1 = 0;
#else
# error "missing implementations for your architecture"
#endif /* __x86_64__ */

  make_user_context(&trampoline->uth.uc, 
                    (void (*) ()) __lithe_task_block,
	                6, func0, func1, task0, task1, arg0, arg1);

  lithe_task_t *task = current_task;
  current_task = trampoline;
  swapcontext(&task->uth.uc, &trampoline->uth.uc);

  return 0;
}

int lithe_task_unblock(lithe_task_t *task)
{
  if (task == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* Explicitly use NULL task. */
  lithe_task_t *t = current_task;
  current_task = NULL;

  lithe_sched_t *sched = current_sched;
  current_sched = task->sched;
  current_sched->funcs->unblock(current_sched->this, task);
  current_sched = sched;

  current_task = t;

  return 0;
}

int lithe_task_resume(lithe_task_t *task)
{
  if (task == NULL) {
    errno = EINVAL;
    return -1;
  }

  /* TODO(benh): Must we be on the trampoline? */
  if (current_task != trampoline) {
    errno = EPERM;
    return -1;
  }

  current_task = task;
  setcontext(&task->uth.uc);
  return -1;
}

