/*
 * Lithe implementation.
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

#include <sys/mman.h>
#include <sys/resource.h>

#include <lithe/lithe.h>
#include <lithe/fatal.h>

#ifndef __linux__
#error "expecting __linux__ to be defined (for now, Lithe only runs on Linux)"
#endif

/* Struct to hold the function pointer and argument of a function to be called
 * in vcore_entry after one of the lithe functions yields from a context */
typedef struct lithe_vcore_func {
  void (*func) (void *);
  void *arg;
} lithe_vcore_func_t;

/* Hooks from uthread code into lithe */
static uthread_t*   lithe_init(void);
static void         lithe_vcore_entry(void);
static void         lithe_thread_runnable(uthread_t *uthread);
static void         lithe_thread_yield(uthread_t *uthread);

/* Unique function pointer table required by uthread code */
struct schedule_ops lithe_sched_ops = {
  .sched_init       = lithe_init,
  .sched_entry      = lithe_vcore_entry,
  .thread_runnable  = lithe_thread_runnable,
  .thread_yield     = lithe_thread_yield,
  .preempt_pending  = NULL, /* lithe_preempt_pending, */
  .spawn_thread     = NULL, /* lithe_spawn_thread, */
};
/* Publish these schedule_ops, overriding the weak defaults setup in uthread */
struct schedule_ops *sched_ops = &lithe_sched_ops;

/* Lithe's base scheduler functions */
static int base_vcore_request(lithe_sched_t *this, lithe_sched_t *child, int k);
static void base_vcore_enter(lithe_sched_t *this);
static void base_vcore_return(lithe_sched_t *this, lithe_sched_t *child);
static void base_child_entered(lithe_sched_t *this, lithe_sched_t *child);
static void base_child_exited(lithe_sched_t *this, lithe_sched_t *child);
static void base_context_block(lithe_sched_t *__this, lithe_context_t *context);
static void base_context_unblock(lithe_sched_t *__this, lithe_context_t *context);
static void base_context_yield(lithe_sched_t *__this, lithe_context_t *context);
static void base_context_exit(lithe_sched_t *__this, lithe_context_t *context);

static const lithe_sched_funcs_t base_funcs = {
  .vcore_request      = base_vcore_request,
  .vcore_enter        = base_vcore_enter,
  .vcore_return       = base_vcore_return,
  .child_entered      = base_child_entered,
  .child_exited       = base_child_exited,
  .context_block      = base_context_block,
  .context_unblock    = base_context_unblock,
  .context_yield      = base_context_yield,
  .context_exit       = base_context_exit
};

/* Base scheduler itself */
static lithe_sched_t base_sched = { 
  .funcs           = &base_funcs,
  .vcores          = 0,
  .parent          = NULL,
  .parent_context  = NULL,
  .start_context   = {{{0}}}
};

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
#define next_func        (lithe_tls.next_func)
#define current_sched    (lithe_tls.current_sched)
#define current_context  ((lithe_context_t*)current_uthread)

/* Helper function for initializing the barebones of a lithe context */
static inline void __lithe_context_init(lithe_context_t *context, lithe_sched_t *sched);

void vcore_ready()
{
  /* Once the vcore subsystem is up and running, initialize the uthread
   * library, which will, in turn, eventually call lithe_init() for us */
  uthread_lib_init();
}

static uthread_t* lithe_init()
{
  /* Create a lithe context for the main thread to run in */
  lithe_context_t *context = (lithe_context_t*)malloc(sizeof(lithe_context_t));
  assert(context);

  /* Fill in the main context stack info with some data. This data is garbage,
   * and only necessary so as to keep lithe_sched_entry from allocating a new
   * stack when creating a context to run the first scheduler it enters. */
  context->stack.bottom = (void*)0xdeadbeef;
  context->stack.size = (ssize_t)-1;

  /* Set the scheduler associated with the context to be the base scheduler */
  current_sched = &base_sched;

  /* Return a reference to the main context back to the uthread library. We will
   * resume this context once lithe_vcore_entry() is called from the uthread
   * library */
  return (uthread_t*)(context);
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

static void lithe_thread_runnable(struct uthread *uthread)
{
  assert(uthread);
  assert(current_sched);
  assert(current_sched->funcs->context_unblock);

  lithe_context_t *context = (lithe_context_t*)uthread;
  current_sched->funcs->context_unblock(current_sched, context);
}

static void lithe_thread_yield(struct uthread *uthread)
{
  assert(uthread);
  assert(current_sched);

  lithe_context_t *context = (lithe_context_t*)uthread;
  if(context->state == CONTEXT_BLOCKED) {
    assert(current_sched->funcs->context_block);
    current_sched->funcs->context_block(current_sched, context);
  }
  else if(context->state == CONTEXT_FINISHED) {
    assert(current_sched->funcs->context_exit);
    current_sched->funcs->context_exit(current_sched, context);
  }
  else if(context->state == CONTEXT_YIELDED) {
    assert(current_sched->funcs->context_yield);
    current_sched->funcs->context_yield(current_sched, context);
  }
  /* Otherwise, just fall through, we have yielded internally for some reasone
   * and don't want to notify the scheduler */
}

static void base_vcore_enter(lithe_sched_t *__this)
{
  assert(root_sched != NULL);
  lithe_vcore_grant(root_sched);
}

static void base_vcore_return(lithe_sched_t *__this, lithe_sched_t *sched)
{
  /* Cleanup tls and yield the vcore back to the system. */
  __sync_fetch_and_add(&base_sched.vcores, -1);
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
  __sync_fetch_and_add(&base_sched.vcores, granted);
  return granted;
}

static void base_context_block(lithe_sched_t *__this, lithe_context_t *context)
{
  fatal("Contexts created by the base scheduler should never need to block!\n");
}

static void base_context_unblock(lithe_sched_t *__this, lithe_context_t *context)
{
  fatal("Contexts created by the base scheduler should never need to unblock!\n");
}

static void base_context_yield(lithe_sched_t *__this, lithe_context_t *context)
{
  fatal("The base context should never be yielding!\n");
}

static void base_context_exit(lithe_sched_t *__this, lithe_context_t *context)
{
  fatal("The base context should never be exiting!\n");
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

  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

  /* Leave child, join parent. */
  __sync_fetch_and_add(&(child->vcores), -1);

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
    lithe_sched_t* child;
    lithe_context_t*  child_context;
  } *real_arg = arg;
  lithe_sched_t* parent      = real_arg->parent;
  lithe_sched_t* child       = real_arg->child;
  lithe_context_t*  child_context  = real_arg->child_context;

  assert(parent);
  assert(child);
  assert(child_context);

  /* Inform parent. */
  assert(parent->funcs->child_entered);
  parent->funcs->child_entered(parent, child);

  /* Return to to the vcore_entry point to continue running the child context now
   * that it has been properly setup */
  next_context = child_context;
  lithe_vcore_entry();
}

int lithe_sched_enter(lithe_sched_t *child)
{
  assert(!in_vcore_context());
  assert(current_sched);

  /* Make sure that the childs 'funcs' have already been set up properly */
  assert(child->funcs);
 
  lithe_sched_t* parent = current_sched;
  lithe_context_t*  parent_context = current_context;

  /* Set-up child scheduler */
  child->vcores = 0;
  child->parent = parent;
  child->parent_context = parent_context;

  /* Update the number of vcores now owned by this child */
  __sync_fetch_and_add(&(child->vcores), 1);

  /* Initialize the child's start context to highjack the current context */
  lithe_context_t *child_context = &child->start_context;
  child_context->stack = current_context->stack;
  __lithe_context_init(child_context, child);

  /* Hijack the current context with the newly created one. */
  set_current_uthread(&child_context->uth);

  /* Update the current scheduler to be the the child. Use the
   * safe_set_tls_var() macros since our call to set_current_uthread() above
   * has changed the tls_desc, and it's not 100% ensured that the gcc optimizer
   * will recognize this. */
  safe_set_tls_var(current_sched, child);
  vcore_set_tls_var(vcore_id(), current_sched, child);

  /* Set up a function to run in vcore context to inform the parent that the
   * child has taken over */
  struct { 
    lithe_sched_t* parent;
    lithe_sched_t* child;
    lithe_context_t* child_context;
  } real_arg = {parent, child, child_context};
  lithe_vcore_func_t real_func = {__lithe_sched_enter, &real_arg};
  vcore_set_tls_var(vcore_id(), next_func, &real_func);

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
  } *real_arg = arg;
  lithe_sched_t* parent      = real_arg->parent;
  lithe_context_t*  parent_context = real_arg->parent_context;
  lithe_sched_t* child       = real_arg->child;

  assert(parent);
  assert(parent_context);
  assert(child);

  /* Inform the parent that this child scheduler has finished */
  assert(parent->funcs->child_exited);
  parent->funcs->child_exited(parent, child);

  /* Return to the original parent context */
  next_context = parent_context;
  lithe_vcore_entry();
}

int lithe_sched_exit()
{
  assert(!in_vcore_context());
  assert(current_sched);

  lithe_sched_t* parent = current_sched->parent;
  lithe_context_t* parent_context = current_sched->parent_context;
  lithe_sched_t *child = current_sched;
  lithe_context_t *child_context = current_context;

  /* Don't actually end this scheduler until all vcores have been yielded
   * (except this one of course) */
  while (coherent_read(child->vcores) != 1);

  /* Update child's vcore count (to 0) */
  __sync_fetch_and_add(&(child->vcores), -1);

  /* Hijack the current context giving it back to the original parent context */
  set_current_uthread(&current_sched->parent_context->uth);

  /* Give control back to the parent */
  current_sched = parent;
  vcore_set_tls_var(vcore_id(), current_sched, current_sched);

  struct { 
    lithe_sched_t* parent;
    lithe_context_t*  parent_context;
    lithe_sched_t* child;
  } real_arg = {parent, parent_context, child};
  lithe_vcore_func_t real_func = {__lithe_sched_exit, &real_arg};
  vcore_set_tls_var(vcore_id(), next_func, &real_func);

  /* Yield this context to vcore context to run the function we just set up. Once
   * we return from the yield we will be fully back in the parent scheduler
   * running the original parent context. */
  uthread_yield(true);

  /* Cleanup the child context we initialized in lithe_sched_entry() */
  lithe_context_cleanup(child_context);

  return 0;
}

int lithe_vcore_request(int k)
{
  assert(current_sched);
  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

  current_sched = parent;
  assert(parent->funcs->vcore_request);
  int granted = parent->funcs->vcore_request(parent, child, k);
  __sync_fetch_and_add(&child->vcores, granted);
  current_sched = child;
  return granted;
}

static void __lithe_context_run()
{
  current_context->start_func(current_context->arg);
  current_context->state = CONTEXT_FINISHED;
  uthread_yield(false);
}

static inline void __lithe_context_fields_init(lithe_context_t *context, lithe_sched_t *sched)
{
  context->start_func = NULL;
  context->arg = NULL;
  context->cls = NULL;
  context->state  = CONTEXT_READY;
  uthread_set_tls_var(&context->uth, current_sched, sched);
}

static inline void __lithe_context_init(lithe_context_t *context, lithe_sched_t *sched)
{
  /* Initialize the new context as a uthread */
  uthread_init((uthread_t*)context);

  /* Initialize the fields associated with a lithe context */
  __lithe_context_fields_init(context, sched);
}

static inline void __lithe_context_reinit(lithe_context_t *context, lithe_sched_t *sched)
{
  /* Renitialize the new context as a uthread */
  uthread_reinit((uthread_t*)context);

  /* Initialize the fields associated with a lithe context */
  __lithe_context_fields_init(context, sched);
}

static inline void __lithe_context_set_entry(lithe_context_t *context, 
                                             void (*func) (void *), void *arg)
{
  assert(context->stack.bottom);
  assert(context->stack.size);

  context->start_func = func;
  context->arg = arg;
  init_uthread_stack(&context->uth, context->stack.bottom, context->stack.size);
  init_uthread_entry(&context->uth, __lithe_context_run);
}

void lithe_context_init(lithe_context_t *context, void (*func) (void *), void *arg)
{
  /* Assert that this is a valid context */
  assert(context);

  /* Call the internal lithe context init */
  __lithe_context_init(context, current_sched);

  /* Initialize context start function and stack */
  __lithe_context_set_entry(context, func, arg);
}

void lithe_context_reinit(lithe_context_t *context, void (*func) (void *), void *arg)
{
  /* Assert that this is a valid context */
  assert(context);

  /* Call the internal lithe context reinit */
  __lithe_context_reinit(context, current_sched);

  /* Initialize context start function and stack */
  __lithe_context_set_entry(context, func, arg);
}

void lithe_context_cleanup(lithe_context_t *context)
{
  assert(context);
  if(!in_vcore_context())
    assert(context != current_context);
  uthread_cleanup(&context->uth);
}

lithe_context_t *lithe_context_self()
{
  return current_context;
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
  current_context->state = CONTEXT_BLOCKED;
  uthread_yield(true);
  current_context->state = CONTEXT_READY;
  return 0;
}

int lithe_context_unblock(lithe_context_t *context)
{
  assert(context);
  uthread_runnable(&context->uth);
  return 0;
}

void lithe_context_yield()
{
  assert(!in_vcore_context());
  assert(current_sched);
  assert(current_context);
  current_context->state = CONTEXT_YIELDED;
  uthread_yield(true);
  current_context->state = CONTEXT_READY;
}

void lithe_context_set_cls(lithe_context_t *context, void *cls) 
{
  assert(context);
  context->cls = cls;
}

void *lithe_context_get_cls(lithe_context_t *context)
{
  assert(context);
  return context->cls;
}

