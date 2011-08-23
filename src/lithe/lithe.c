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
  /* Number of vcores currently owned by this scheduler. */
  int vcores;

  /* Pointer to the start context of this scheduler */
  lithe_context_t *start_context;

  /* Scheduler's parent scheduler */
  lithe_sched_t *parent;

  /* Parent context from which this scheduler was started. */
  lithe_context_t *parent_context;

};

/* Struct to hold the function pointer and argument of a function to be called
 * in vcore_entry after one of the lithe functions yields from a context */
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
static void         lithe_thread_destroy(uthread_t *uthread);

/* Unique function pointer table required by uthread code */
struct schedule_ops lithe_sched_ops = {
  .sched_init       = lithe_init,
  .sched_entry      = lithe_vcore_entry,
  .thread_create    = lithe_thread_create,
  .thread_runnable  = lithe_thread_runnable,
  .thread_yield     = lithe_thread_yield,
  .thread_destroy   = lithe_thread_destroy,
  .preempt_pending  = NULL, /* lithe_preempt_pending, */
  .spawn_thread     = NULL, /* lithe_spawn_thread, */
};
/* Publish these schedule_ops, overriding the weak defaults setup in uthread */
struct schedule_ops *sched_ops __attribute__((weak)) = &lithe_sched_ops;

/* Lithe's base scheduler functions */
static int base_vcore_request(lithe_sched_t *this, lithe_sched_t *child, int k);
static void base_vcore_enter(lithe_sched_t *this);
static void base_vcore_return(lithe_sched_t *this, lithe_sched_t *child);
static void base_child_entered(lithe_sched_t *this, lithe_sched_t *child);
static void base_child_exited(lithe_sched_t *this, lithe_sched_t *child);
static lithe_context_t* base_context_create(lithe_sched_t *__this, lithe_context_attr_t *attr);
static void base_context_destroy(lithe_sched_t *__this, lithe_context_t *context);
static void base_context_stack_create(lithe_sched_t *__this, lithe_context_stack_t *stack);
static void base_context_stack_destroy(lithe_sched_t *__this, lithe_context_stack_t *stack);
static void base_context_yield(lithe_sched_t *__this, lithe_context_t *context);
static void base_context_runnable(lithe_sched_t *__this, lithe_context_t *context);

static const lithe_sched_funcs_t base_funcs = {
  .vcore_request      = base_vcore_request,
  .vcore_enter        = base_vcore_enter,
  .vcore_return       = base_vcore_return,
  .child_entered      = base_child_entered,
  .child_exited       = base_child_exited,
  .context_create        = base_context_create,
  .context_destroy       = base_context_destroy,
  .context_stack_create  = base_context_stack_create,
  .context_stack_destroy = base_context_stack_destroy,
  .context_runnable      = base_context_runnable,
  .context_yield         = base_context_yield
};

static lithe_sched_idata_t base_idata = {
  .vcores       = 0,
  .parent       = NULL,
  .parent_context  = NULL,
};

/* Base scheduler itself */
static lithe_sched_t base_sched = { 
  .funcs        = &base_funcs,
  .idata        = &base_idata
};

/* Reference to the context running the main thread. */ 
static lithe_context_t *main_context = NULL;

/* Root scheduler, i.e. the child scheduler of base. */
static lithe_sched_t *root_sched = NULL;

static __thread struct {
  /* The next context to run on this vcore when lithe_vcore_entry is called again
   * after a yield from another lithe context */
  lithe_context_t *next_context;

  /* The next function to run on this vcore when lithe_vcore_entry is called
   * after a yield from a lithe context */
  lithe_vcore_func_t *next_func;

  /* The current scheduler to run any contexts / functions with */
  lithe_sched_t *current_sched;

} lithe_tls = {NULL, NULL, NULL};
#define next_context     (lithe_tls.next_context)
#define next_func     (lithe_tls.next_func)
#define current_sched (lithe_tls.current_sched)
#define current_context  ((lithe_context_t*)current_uthread)

void vcore_ready()
{
  /* Once the vcore subsystem is up and running, initialize the uthread
   * library, which will, in turn, eventually call lithe_init() for us */
  uthread_init();
}

static uthread_t* lithe_init()
{
  /* Create a lithe context for the main thread to run in */
  main_context = (lithe_context_t*)calloc(1, sizeof(lithe_context_t));
  assert(main_context);

  /* Fill in the main context stack info with some data. This data is garbage,
   * and only necessary so as to keep lithe_sched_entry from allocating a new
   * stack when creating a context to run the first scheduler it enters. */
  main_context->stack.bottom = (void*)0xdeadbeef;
  main_context->stack.size = (ssize_t)-1;

  /* Set the scheduler associated with the context to be the base scheduler */
  current_sched = &base_sched;

  /* Return a reference to the main context back to the uthread library. We will
   * resume this context once lithe_vcore_entry() is called from the uthread
   * library */
  return (uthread_t*)(main_context);
}

static void __attribute__((noreturn)) __lithe_sched_reenter()
{
  assert(in_vcore_context());

  /* Enter current scheduler. */
  assert(current_sched->funcs->vcore_enter);
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
  }

  /* If current_context is set, then just resume it. This will happen in one of 2
   * cases: 1) It is the first, i.e. main thread, or 2) The current vcore was
   * taken over to run a signal handler from the kernel, and is now being
   * restarted */
  if(current_context) {
    current_sched = uthread_get_tls_var(&current_context->uth, current_sched);
    run_current_uthread();
    assert(0); // Should never return from running context
  }

  /* Otherwise, if next_context is set, start it off anew. */
  if(next_context) {
    lithe_context_t *context = next_context;
    current_sched = uthread_get_tls_var(&context->uth, current_sched);
    next_context = NULL;
    run_uthread(&context->uth);
    assert(0); // Should never return from running context
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

static uthread_t* lithe_thread_create(void (*func)(void), void *udata)
{
  assert(current_sched);
  assert(current_sched->funcs->context_create);

  lithe_context_t* context = current_sched->funcs->context_create(current_sched, udata);
  assert(context);

  return (uthread_t*)context;
}

static void lithe_thread_destroy(struct uthread *uthread)
{
  assert(uthread);
  assert(current_sched);
  assert(current_sched->funcs->context_destroy);

  lithe_context_t *context = (lithe_context_t*)uthread;
  current_sched->funcs->context_destroy(current_sched, context);
}

static void lithe_thread_runnable(struct uthread *uthread)
{
  assert(uthread);
  assert(current_sched);
  assert(current_sched->funcs->context_runnable);

  lithe_context_t *context = (lithe_context_t*)uthread;
  current_sched->funcs->context_runnable(current_sched, context);
}

static void lithe_thread_yield(struct uthread *uthread)
{
  assert(uthread);
  assert(current_sched);

  lithe_context_t *context = (lithe_context_t*)uthread;
  if(context->finished) {
    assert(current_sched->funcs->context_destroy);
    uthread_destroy(uthread);
  }
  else {
    assert(current_sched->funcs->context_yield);
    current_sched->funcs->context_yield(current_sched, context);
  }
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

static void base_child_entered(lithe_sched_t *__this, lithe_sched_t *sched)
{
  assert(root_sched == NULL);
  root_sched = sched;
}

static void base_child_exited(lithe_sched_t *__this, lithe_sched_t *sched)
{
  assert(root_sched == sched);
  root_sched = NULL;
}

static int base_vcore_request(lithe_sched_t *__this, lithe_sched_t *sched, int k)
{
  assert(root_sched == sched);
  int granted = vcore_request(k);
  __sync_fetch_and_add(&base_sched.idata->vcores, granted);
  return granted;
}

static lithe_context_t* base_context_create(lithe_sched_t *__this, lithe_context_attr_t *attr)
{
  fatal("The base scheduler should never be creating contexts of its own!\n");
  return NULL;
}

static void base_context_destroy(lithe_sched_t *__this, lithe_context_t *context)
{
  fatal("The base scheduler should never have any contexts to be destroying!\n");
}

static void base_context_stack_create(lithe_sched_t *__this, lithe_context_stack_t *stack)
{
  fatal("The base scheduler should never be creating context stacks of its own!\n");
}

static void base_context_stack_destroy(lithe_sched_t *__this, lithe_context_stack_t *stack)
{
  fatal("The base scheduler should never have any context stacks to be destroying!\n");
}

static void base_context_yield(lithe_sched_t *__this, lithe_context_t *context)
{
  // Do nothing.  The only context we should ever yield is the main context...
}

static void base_context_runnable(lithe_sched_t *__this, lithe_context_t *context)
{
  fatal("Contexts created by the base scheduler should never need to be made runnable!\n");
}

lithe_sched_t *lithe_sched_current()
{
  return current_sched;
}

int lithe_vcore_grant(lithe_sched_t *child)
{
  assert(child);
  assert(in_vcore_context());

  /* Leave parent, join child. */
  assert(child != &base_sched);
  current_sched = child;

  /* Enter the child scheduler */
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

  /* Yield the vcore to the parent */
  current_sched = parent;
  assert(current_sched->funcs->vcore_return);
  current_sched->funcs->vcore_return(parent, child);
  __lithe_sched_reenter();
}

static void __lithe_sched_enter(void *arg)
{
  assert(in_vcore_context());

  /* Unpack the real arguments to this function */
  struct { 
    lithe_sched_t* parent;
    lithe_context_t*  parent_context;
    lithe_sched_t* child;
    lithe_context_t*  child_context;
  } *real_arg = arg;
  lithe_sched_t* parent      = real_arg->parent;
  lithe_context_t*  parent_context = real_arg->parent_context;
  lithe_sched_t* child       = real_arg->child;
  lithe_context_t*  child_context  = real_arg->child_context;

  assert(parent);
  assert(parent_context);
  assert(child);
  assert(child_context);

  /* Create the childs sched_idata and set up the pointer to it */
  lithe_sched_idata_t *child_idata = (lithe_sched_idata_t*)malloc(sizeof(lithe_sched_idata_t));
  child->idata = child_idata;

  /* Set-up child scheduler */
  child->idata->vcores = 0;
  child->idata->parent = parent;
  child->idata->parent_context = parent_context;

  /* Update the current scheduler to be the the child */
  current_sched = child;
  uthread_set_tls_var(child_context, current_sched, current_sched);

  /* Update the number of vcores now owned by this child */
  __sync_fetch_and_add(&(child->idata->vcores), 1);

  /* Inform parent. */
  assert(parent->funcs->child_entered);
  parent->funcs->child_entered(parent, child);

  /* Return to to the vcore_entry point to continue running the child context now
   * that it has been properly setup */
  next_context = child_context;
  lithe_vcore_entry();
}

int lithe_sched_enter(const lithe_sched_funcs_t *funcs, lithe_sched_t *child)
{
  assert(funcs);
  assert(!in_vcore_context());
  assert(current_sched);

  /* Associate the constant scheduler funcs with the child scheduler */
  child->funcs = funcs;
 
  lithe_sched_t* parent = current_sched;
  lithe_context_t*  parent_context = current_context;

  /* Create a child context to highjack the current context's context */
  assert(current_context->stack.bottom);
  assert(current_context->stack.size);
  lithe_context_attr_t attr;
  attr.stack = current_context->stack;
  current_sched = child;
  lithe_context_t *child_context = lithe_context_create(&attr, NULL, NULL);
  current_sched = parent;

  /* Set up a function to run in vcore context to actually setup and start the
   * child scheduler in the context we are about to highjack */
  struct { 
    lithe_sched_t* parent;
    lithe_context_t*  parent_context;
    lithe_sched_t* child;
    lithe_context_t*  child_context;
  } real_arg = {parent, parent_context, child, child_context};
  lithe_vcore_func_t real_func = {__lithe_sched_enter, &real_arg};
  vcore_set_tls_var(vcore_id(), next_func, &real_func);

  /* Hijack the current context with the newly created one. */
  set_current_uthread(&child_context->uth);

  /* Yield this context to vcore context to run the function we just set up. Once
   * we return from the yield we will be fully inside the child scheduler
   * running the child context. */
  uthread_yield(true);
  return 0;
}

void __lithe_sched_exit(void *arg)
{
  struct { 
    lithe_sched_t* parent;
    lithe_context_t*  parent_context;
    lithe_sched_t* child;
    lithe_context_t*  child_context;
  } *real_arg = arg;
  lithe_sched_t* parent      = real_arg->parent;
  lithe_context_t*  parent_context = real_arg->parent_context;
  lithe_sched_t* child       = real_arg->child;
  lithe_context_t*  child_context  = real_arg->child_context;

  assert(parent);
  assert(parent_context);
  assert(child);
  assert(child_context);

  /* Don't actually end this scheduler until all vcores have been yielded
   * (except this one of course) */
  while (coherent_read(child->idata->vcores) != 1);

  /* Update child's vcore count (to 0) */
  __sync_fetch_and_add(&(child->idata->vcores), -1);

  /* Symetrically, the child context should be destroyed in the 'real'
   * lithe_context_exit() since it was created in the 'real' lithe_context_enter(),
   * but doing so would require us to retain a superfluous reference to it in
   * the body of lithe_context_exit().  To avoid this, we just destroy it here. */
  lithe_context_destroy(child_context);

  /* Give control back to the parent */
  current_sched = parent;
  uthread_set_tls_var(parent_context, current_sched, current_sched);

  /* Inform the parent that this child scheduler has finished */
  assert(parent->funcs->child_exited);
  parent->funcs->child_exited(parent, child);

  /* Destroy the child's idata */
  free(child->idata);

  /* Return to the original parent context */
  next_context = parent_context;
  lithe_vcore_entry();
}

int lithe_sched_exit()
{
  assert(!in_vcore_context());
  assert(current_sched);

  struct { 
    lithe_sched_t* parent;
    lithe_context_t*  parent_context;
    lithe_sched_t* child;
    lithe_context_t*  child_context;
  } real_arg = {current_sched->idata->parent, current_sched->idata->parent_context, 
                current_sched, current_context};
  lithe_vcore_func_t real_func = {__lithe_sched_exit, &real_arg};
  vcore_set_tls_var(vcore_id(), next_func, &real_func);

  /* Hijack the current context giving it back to the original parent context */
  set_current_uthread(&current_sched->idata->parent_context->uth);

  /* Yield this context to vcore context to run the function we just set up. Once
   * we return from the yield we will be fully back in the parent scheduler
   * running the original parent context. */
  uthread_yield(true);
  return 0;
}

int lithe_vcore_request(int k)
{
  assert(current_sched);
  lithe_sched_t *parent = current_sched->idata->parent;
  lithe_sched_t *child = current_sched;

  current_sched = parent;
  assert(parent->funcs->vcore_request);
  int granted = parent->funcs->vcore_request(parent, child, k);
  __sync_fetch_and_add(&child->idata->vcores, granted);
  current_sched = child;
  return granted;
}

static void __lithe_context_run()
{
  current_context->start_func(current_context->arg);
  uthread_yield(false);
}

static void __lithe_context_init_bare(lithe_context_t *context, lithe_context_attr_t *attr)
{
  uthread_set_tls_var(&context->uth, current_sched, current_sched);
  context->stack = attr->stack;
  context->tls = NULL;
  context->finished = false;
  context->free_stack = false;
}

void lithe_context_attr_init(lithe_context_attr_t *attr)
{
  attr->stack.bottom = 0;
  attr->stack.size = 0;
}

lithe_context_t *lithe_context_create(lithe_context_attr_t *__attr, void (*func) (void *), void *arg) 
{
  /* If attr is NULL, then create one and initialize it so it can be used
   * properly in the context initialization functions to follow */
  lithe_context_attr_t *attr;
  if(__attr == NULL) {
    lithe_context_attr_t _attr;
    lithe_context_attr_init(&_attr);
    attr = &_attr;
  }
  /* Otherwise, just use the attr that was passed in */
  else attr = __attr;

  /* Create the new context */
  lithe_context_t *context = (lithe_context_t*)uthread_create(NULL, &attr);
  assert(context);

  /* If a stack bottom is already set in the attributes, then just pass this
   * along as the stack to use for the newly created context */
  bool created_stack = false;
  if(attr->stack.bottom) {
    assert(attr->stack.size);
    context->stack = attr->stack;
  }
  /* Otherwise, create a new stack for the context */
  else {
    assert(current_sched->funcs->context_stack_create);
    current_sched->funcs->context_stack_create(current_sched, &attr->stack);
    context->stack = attr->stack;
    created_stack = true;
  }

  /* If a func was passed in, go through the full initialization process. In
   * both cases, the stack for the context MUST already have been setup somehow */
  if(func)
    lithe_context_init(context, attr, func, arg);
  /* Otherwise do a bare initialization */
  else
    __lithe_context_init_bare(context, attr);

  /* Need to set context->free_stack AFTER calling lithe_context_init(). Otherwise it
   * gets reset in the body of that function */
  if(created_stack)
    context->free_stack = true;

  assert(context->stack.bottom);
  assert(context->stack.size);
  return context;
}

void lithe_context_init(lithe_context_t *context, lithe_context_attr_t *attr, void (*func) (void *), void *arg)
{
  assert(context);
  assert(attr);
  assert(attr->stack.bottom);
  assert(attr->stack.size);

  __lithe_context_init_bare(context, attr);
  context->start_func = func;
  context->arg = arg;

  init_uthread_stack(&context->uth, context->stack.bottom, context->stack.size);
  init_uthread_entry(&context->uth, __lithe_context_run);
}

void lithe_context_destroy(lithe_context_t *context)
{
  assert(context);
  if(!in_vcore_context())
    assert(context != current_context);
  if(context->free_stack) {
    assert(current_sched->funcs->context_stack_destroy);
    current_sched->funcs->context_stack_destroy(current_sched, &context->stack);
  }
  uthread_destroy(&context->uth);
}

lithe_context_t *lithe_context_self()
{
  return current_context;
}

void lithe_context_settls(void *tls) 
{
  assert(current_context);
  assert(!in_vcore_context());
  current_context->tls = tls;
}

void *lithe_context_gettls()
{
  assert(current_context);
  assert(!in_vcore_context());
  return current_context->tls;
}

int lithe_context_run(lithe_context_t *context)
{
  assert(context);
  assert(in_vcore_context());

  next_context = context;
  lithe_vcore_entry();
  return -1;
}

void __lithe_context_block(void *arg)
{
  assert(arg);
  assert(in_vcore_context());

  struct { 
    void (*func) (lithe_context_t *, void *); 
    lithe_context_t *context;
    void *arg;
  } *real_arg = arg;

  real_arg->func(real_arg->context, real_arg->arg);
  __lithe_sched_reenter();
}

int lithe_context_block(void (*func) (lithe_context_t *, void *), void *arg)
{
  assert(func);
  assert(!in_vcore_context());
  assert(current_context);

  struct { 
    void (*func) (lithe_context_t *, void *); 
    lithe_context_t *context;
    void *arg;
  } real_arg = {func, current_context, arg};
  lithe_vcore_func_t real_func = {__lithe_context_block, &real_arg};

  vcore_set_tls_var(vcore_id(), next_func, &real_func);
  uthread_yield(true);
  return 0;
}

int lithe_context_unblock(lithe_context_t *context)
{
  assert(context);
  assert(current_sched->funcs->context_runnable);
  current_sched->funcs->context_runnable(current_sched, context);
  return 0;
}

void lithe_context_yield()
{
  assert(!in_vcore_context());
  assert(current_sched);
  assert(current_context);
  uthread_yield(true);
}

void lithe_context_exit()
{
  assert(!in_vcore_context());
  assert(current_sched);
  assert(current_context);
  current_context->finished = true;
  uthread_yield(false);
}

