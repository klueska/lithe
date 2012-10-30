/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/*
 * Lithe implementation.
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <parlib/parlib.h>
#include "lithe.h"
#include "fatal.h"

#ifndef __linux__
#ifndef __ros__
#error "Sorry, your operating system is not yet support by lithe (Linux and Akaros, currently)"
#endif
#endif

/* Define the functions for maintaining a queue of lithe contexts */
DEFINE_TYPED_DEQUE(lithe_context, lithe_context_t *);

/* Struct to hold the function pointer and argument of a function to be called
 * in vcore_entry after one of the lithe functions yields from a context */
typedef struct lithe_vcore_func {
  void (*func) (void *);
  void *arg;
} lithe_vcore_func_t;

/* Hooks from uthread code into lithe */
static void         lithe_vcore_entry(void);
static void         lithe_thread_runnable(uthread_t *uthread);

/* Unique function pointer table required by uthread code */
struct schedule_ops lithe_sched_ops = {
  .sched_entry      = lithe_vcore_entry,
  .thread_runnable  = lithe_thread_runnable,
  .preempt_pending  = NULL, /* lithe_preempt_pending, */
  .spawn_thread     = NULL, /* lithe_spawn_thread, */
};
/* Publish these schedule_ops, overriding the weak defaults setup in uthread */
struct schedule_ops *sched_ops = &lithe_sched_ops;

/* Lithe's base scheduler functions */
static int base_hart_request(lithe_sched_t *this, lithe_sched_t *child, int k);
static void base_hart_enter(lithe_sched_t *this);
static void base_hart_return(lithe_sched_t *this, lithe_sched_t *child);
static void base_child_entered(lithe_sched_t *this, lithe_sched_t *child);
static void base_child_exited(lithe_sched_t *this, lithe_sched_t *child);
static void base_context_block(lithe_sched_t *__this, lithe_context_t *context);
static void base_context_unblock(lithe_sched_t *__this, lithe_context_t *context);
static void base_context_yield(lithe_sched_t *__this, lithe_context_t *context);
static void base_context_exit(lithe_sched_t *__this, lithe_context_t *context);

static const lithe_sched_funcs_t base_funcs = {
  .hart_request       = base_hart_request,
  .hart_enter         = base_hart_enter,
  .hart_return        = base_hart_return,
  .child_enter        = base_child_entered,
  .child_exit         = base_child_exited,
  .context_block      = base_context_block,
  .context_unblock    = base_context_unblock,
  .context_yield      = base_context_yield,
  .context_exit       = base_context_exit
};

/* Base scheduler itself */
static lithe_sched_t base_sched = { 
  .funcs           = &base_funcs,
  .harts          = 0,
  .parent          = NULL,
  .parent_context  = NULL,
  .start_context   = {0}
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
/* Helper function for determining which state to resume from after yielding */
static void __lithe_context_yield(uthread_t *uthread, void *arg);
/* Helper function for deleting all of the cls associated with a lithe context */
static void __destroy_cls();

static int __attribute__((constructor)) lithe_lib_init()
{
  /* Make sure this only runs once */
  static bool initialized = false;
  if (initialized)
      return -1;
  initialized = true;

  /* Create a lithe context for the main thread to run in */
  lithe_context_t *context = (lithe_context_t*)malloc(sizeof(lithe_context_t));
  assert(context);

  /* Fill in the main context stack info with some data. This data is garbage,
   * and only necessary so as to keep lithe_sched_entry from allocating a new
   * stack when creating a context to run the first scheduler it enters. */
  context->stack.bottom = (void*)0xdeadbeef;
  context->stack.size = (ssize_t)-1;

  /* Set the scheduler associated with the context to be the base scheduler */
  context->sched = &base_sched;

  /* Once we have set things up for the main context, initialize the uthread
   * library with that main context */
  assert(!uthread_lib_init((uthread_t*)context));

  /* Now that the library is initialized, a TLS shoudl eb set up for this
   * context, so set it's current_sched var */
  uthread_set_tls_var(&context->uth, current_sched, &base_sched)

  return 0;
}

static void __attribute__((noreturn)) __lithe_sched_reenter()
{
  assert(in_vcore_context());
  assert(current_sched);
  assert(current_sched->funcs);

  /* Enter current scheduler. */
  assert(current_sched->funcs->hart_enter);
  current_sched->funcs->hart_enter(current_sched);
  fatal("lithe: returned from enter");
}

static void __attribute__((noreturn)) lithe_vcore_entry()
{
  /* Make sure we are in vcore context */
  assert(in_vcore_context());

  /* If we are entering this vcore for the first time, we need to set the
   * current scheduler appropriately and up the hart count for it */
  if(current_sched == NULL) {
    /* Set the current scheduler as the base scheduler */
    current_sched = &base_sched;
    __sync_fetch_and_add(&base_sched.harts, 1);
  }

  /* If current_context is set, then just resume it. This will happen in one of 2
   * cases: 1) It is the first, i.e. main thread, or 2) The current vcore was
   * taken over to run a signal handler from the kernel, and is now being
   * restarted */
  if(current_context) {
    current_sched = current_context->sched;
    run_current_uthread();
    assert(0); // Should never return from running context
  }

  /* Otherwise, if next_context is set, start it off anew. */
  if(next_context) {
    lithe_context_t *context = next_context;
    current_sched = context->sched;
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

  /* Otherwise, just reenter hart_enter of whatever the current scheduler is */
  __lithe_sched_reenter();
  assert(0); // Should never return from entered scheduler
}

static void lithe_thread_runnable(uthread_t *uthread)
{
  assert(uthread);
  assert(current_sched);
  assert(current_sched->funcs->context_unblock);

  lithe_context_t *context = (lithe_context_t*)uthread;
  current_sched->funcs->context_unblock(current_sched, context);
}

static void base_hart_enter(lithe_sched_t *__this)
{
  if(root_sched == NULL)
    vcore_yield(false);
  lithe_hart_grant(root_sched, NULL, NULL);
}

static void base_hart_return(lithe_sched_t *__this, lithe_sched_t *sched)
{
  /* Cleanup tls and yield the vcore back to the system. */
  __sync_fetch_and_add(&base_sched.harts, -1);
  memset(&lithe_tls, 0, sizeof(lithe_tls));
  vcore_yield(false);
  /* I should ONLY get here if vcore_yield() decided to return for some reason */
  lithe_vcore_entry();
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

static int base_hart_request(lithe_sched_t *__this, lithe_sched_t *sched, int k)
{
  assert(root_sched == sched);
  return vcore_request(k);
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

void lithe_hart_grant(lithe_sched_t *child, void (*unlock_func) (void *), void *lock)
{
  assert(child);
  assert(in_vcore_context());

  /* Leave parent, join child. */
  assert(child != &base_sched);
  __sync_fetch_and_add(&current_sched->harts, -1);
  __sync_fetch_and_add(&child->harts, 1);
  current_sched = child;
  if(unlock_func != NULL)
    unlock_func(lock);

  /* Enter the child scheduler */
  __lithe_sched_reenter();
  fatal("lithe: returned from enter");
}

void lithe_hart_yield()
{
  assert(in_vcore_context());
  assert(current_sched != &base_sched);

  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

  /* Leave child, join parent. */
  __sync_fetch_and_add(&(child->harts), -1);
  __sync_fetch_and_add(&(parent->harts), 1);

  /* Yield the vcore to the parent */
  current_sched = parent;
  assert(current_sched->funcs->hart_return);
  current_sched->funcs->hart_return(parent, child);
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

  /* Officially grant the core to the child */
  __sync_fetch_and_add(&(parent->harts), -1);
  __sync_fetch_and_add(&(child->harts), 1);

  /* Inform parent. */
  assert(parent->funcs->child_enter);
  parent->funcs->child_enter(parent, child);

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
  child->harts = 0;
  child->parent = parent;
  child->parent_context = parent_context;

  /* Initialize the child's start context to highjack the current context */
  lithe_context_t *child_context = &child->start_context;
  child_context->stack = current_context->stack;
  __lithe_context_init(child_context, child);

  /* Hijack the current context with the newly created one. */
  highjack_current_uthread(&child_context->uth);

  /* Set up a function to run in vcore context to inform the parent that the
   * child has taken over */
  struct { 
    lithe_sched_t* parent;
    lithe_sched_t* child;
    lithe_context_t* child_context;
  } real_arg = {parent, child, child_context};
  lithe_vcore_func_t real_func = {__lithe_sched_enter, &real_arg};
  vcore_set_tls_var(next_func, &real_func);

  /* Yield this context to vcore context to run the function we just set up. Once
   * we return from the yield we will be fully inside the child scheduler
   * running the child context. */
  uthread_yield(true, __lithe_context_yield, NULL);
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

  /* Inform the parent that the child scheduler is about to exit */
  assert(parent->funcs->child_exit);
  parent->funcs->child_exit(parent, child);

  /* Don't actually end the child scheduler until all its vcores have been
   * yielded, except this one of course. This field is synchronized with an
   * update to its value in lithe_hart_grant() as protected by a
   * parent-scheduler-specific locking function. */
  while (child->harts != 1) 
    cpu_relax();

  /* Update child's hart count to 0 */
  __sync_fetch_and_add(&(child->harts), -1);
  __sync_fetch_and_add(&(parent->harts), 1);

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

  /* Hijack the current context so that the yield point below will save the
   * current context into the parent context */
  highjack_current_uthread(&current_sched->parent_context->uth);

  /* Set ourselves up to jump to vcore context and run __lithe_sched_exit() */
  struct { 
    lithe_sched_t* parent;
    lithe_context_t*  parent_context;
    lithe_sched_t* child;
  } real_arg = {parent, parent_context, child};
  lithe_vcore_func_t real_func = {__lithe_sched_exit, &real_arg};
  vcore_set_tls_var(next_func, &real_func);

  /* Yield this context to vcore context to run the function we just set up. Once
   * we return from the yield we will be fully back in the parent scheduler
   * running the original parent context. */
  uthread_yield(true, __lithe_context_yield, NULL);

  /* Cleanup the child context we initialized in lithe_sched_entry() */
  lithe_context_cleanup(child_context);

  return 0;
}

int lithe_hart_request(int k)
{
  assert(current_sched);
  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

  current_sched = parent;
  assert(parent->funcs->hart_request);
  int ret = parent->funcs->hart_request(parent, child, k);
  current_sched = child;
  return ret;
}

static void __lithe_context_start()
{
  assert(current_context);
  assert(current_context->start_func);
  current_context->start_func(current_context->arg);
  __destroy_cls();
  safe_get_tls_var(current_context)->state = CONTEXT_FINISHED;
  uthread_yield(false, __lithe_context_yield, NULL);
}

static inline void __lithe_context_fields_init(lithe_context_t *context, lithe_sched_t *sched)
{
  context->start_func = NULL;
  context->arg = NULL;
  context->state  = CONTEXT_READY;
  context->sched = sched;
  TAILQ_INIT(&context->cls_list);
  uthread_set_tls_var(&context->uth, current_sched, sched)
}

static inline void __lithe_context_reinit(lithe_context_t *context, lithe_sched_t *sched)
{
  /* Renitialize the new context as a uthread */
  uthread_init(&context->uth);

  /* Initialize the fields associated with a lithe context */
  __lithe_context_fields_init(context, sched);
}

static inline void __lithe_context_init(lithe_context_t *context, lithe_sched_t *sched)
{
  /* Zero out the uthread struct before passing it down: required by
   * uthread_init */
  memset(&context->uth, 0, sizeof(uthread_t));
  __lithe_context_reinit(context, sched);
}

static inline void __lithe_context_set_entry(lithe_context_t *context, 
                                             void (*func) (void *), void *arg)
{
  assert(context->stack.bottom);
  assert(context->stack.size);

  context->start_func = func;
  context->arg = arg;
  init_uthread_tf(&context->uth, __lithe_context_start, 
                  context->stack.bottom, context->stack.size);
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

  vcore_set_tls_var(next_func, &real_func);
  current_context->state = CONTEXT_BLOCKED;
  uthread_yield(true, __lithe_context_yield, NULL);
  safe_get_tls_var(current_context)->state = CONTEXT_READY;
  return 0;
}

int lithe_context_unblock(lithe_context_t *context)
{
  assert(context);
  uthread_runnable(&context->uth);
  return 0;
}

static void __lithe_context_yield(uthread_t *uthread, void *arg)
{
  assert(uthread);
  assert(current_sched);
  assert(in_vcore_context());

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
  /* Otherwise, just fall through, we have yielded internally for some reason
   * and don't want to notify the scheduler */
}

void lithe_context_yield()
{
  assert(!in_vcore_context());
  assert(current_sched);
  assert(current_context);
  current_context->state = CONTEXT_YIELDED;
  uthread_yield(true, __lithe_context_yield, NULL);
  safe_get_tls_var(current_context)->state = CONTEXT_READY;
}

static void __destroy_cls() {
  lithe_cls_list_element_t *e = NULL;
  assert(current_context);
  TAILQ_FOREACH(e, &current_context->cls_list, link) {
    lithe_clskey_t *key = e->key;
    bool run_dtor = false;
    spinlock_lock(&key->lock);
    if(key->valid)
      if(key->dtor)
        run_dtor = true;
    spinlock_unlock(&key->lock);
	// MUST run the dtor outside the spinlock if we want it to be able to call
	// code that may deschedule it for a while (i.e. a lithe_mutex). Probably a
	// good idea anyway since it can be arbitrarily long and is written by the
	// user. Note, there is a small race here on the valid field, whereby we
	// may run a destructor on an invalid key. At least the keys memory wont
	// be deleted though, as protected by the ref count. Any reasonable usage
	// of this interface should safeguard that a key is never destroyed before
	// all of the threads that use it have exited anyway.
    if(run_dtor)
      key->dtor(key);
    spinlock_lock(&key->lock);
    key->ref_count--;
    spinlock_unlock(&key->lock);
    if(key->ref_count == 0)
      free(key);
  }
}

lithe_clskey_t *lithe_clskey_create(lithe_cls_dtor_t dtor)
{
  lithe_clskey_t *key = malloc(sizeof(lithe_clskey_t));
  assert(key);

  spinlock_init(&key->lock);
  key->ref_count = 1;
  key->valid = true;
  key->dtor = dtor;
  return key;
}

void lithe_clskey_delete(lithe_clskey_t *key)
{
  assert(key);

  spinlock_lock(&key->lock);
  key->valid = false;
  key->ref_count--;
  spinlock_unlock(&key->lock);
  if(key->ref_count == 0)
    free(key);
}

void lithe_context_set_cls(lithe_clskey_t *key, void *cls)
{
  assert(key);
  assert(current_context);

  spinlock_lock(&key->lock);
  key->ref_count++;
  spinlock_unlock(&key->lock);

  lithe_cls_list_element_t *e = NULL;
  TAILQ_FOREACH(e, &current_context->cls_list, link)
    if(e->key == key) break;
  if(!e) {
    e = malloc(sizeof(lithe_cls_list_element_t));
    assert(e);
    e->key = key;
    TAILQ_INSERT_HEAD(&current_context->cls_list, e, link);
  }
  e->cls = cls;
}

void *lithe_context_get_cls(lithe_clskey_t *key)
{
  assert(key);
  assert(current_context);

  lithe_cls_list_element_t *e = NULL;
  TAILQ_FOREACH(e, &current_context->cls_list, link)
    if(e->key == key) return e->cls;
  return e;
}

