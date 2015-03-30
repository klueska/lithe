/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/*
 * Lithe implementation.
 */

#ifndef __GNUC__
#error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <parlib/parlib.h>
#include "lithe.h"
#include "fatal.h"
#include "internal/assert.h"
#include "internal/vcore.h"

#ifndef __linux__
#ifndef __ros__
#error "Sorry, your operating system is not yet support by lithe (Linux and Akaros, currently)"
#endif
#endif

/* Struct to hold the function pointer and argument of a function to be called
 * in vcore_entry after one of the lithe functions yields from a context */
typedef struct lithe_vcore_func {
  void (*func) (void *);
  void *arg;
} lithe_vcore_func_t;

/* Linux Specific! (handle async syscall events) */
static void lithe_handle_syscall(struct event_msg *ev_msg, unsigned int ev_type);

/* Hooks from uthread code into lithe */
static void lithe_vcore_entry(void);
static void lithe_thread_runnable(uthread_t *uthread);
static void lithe_thread_paused(struct uthread *uthread);
static void lithe_thread_has_blocked(struct uthread *uthread, int flags);
static void lithe_blockon_sysc(struct uthread *uthread, void *sysc);

/* Unique function pointer table required by uthread code */
struct schedule_ops lithe_sched_ops = {
  .sched_entry         = lithe_vcore_entry,
  .thread_runnable     = lithe_thread_runnable,
  .thread_paused       = lithe_thread_paused,
  .thread_has_blocked  = lithe_thread_has_blocked,
  .thread_blockon_sysc = lithe_blockon_sysc,
  .preempt_pending     = NULL,
  .spawn_thread        = NULL,
};

/* A statically defined global to store the main context */
static lithe_context_t lithe_main_context;

/* A statically defined global used to assign the next id to a newly
 * initialized context */
static size_t next_context_id = 0;

/* Lithe's base scheduler functions */
static void base_hart_request(lithe_sched_t *this, lithe_sched_t *child, int h);
static void base_hart_enter(lithe_sched_t *this);
static void base_hart_return(lithe_sched_t *this, lithe_sched_t *child);
static void base_sched_entered(lithe_sched_t *this);
static void base_sched_exited(lithe_sched_t *this);
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
  .sched_enter        = base_sched_entered,
  .sched_exit         = base_sched_exited,
  .child_enter        = base_child_entered,
  .child_exit         = base_child_exited,
  .context_block      = base_context_block,
  .context_unblock    = base_context_unblock,
  .context_yield      = base_context_yield,
  .context_exit       = base_context_exit
};

/* Base scheduler itself */
static lithe_sched_t base_sched = { 
  .funcs  = &base_funcs,
  .harts  = ATOMIC_INITIALIZER(0),
  .parent = NULL,
};

/* Root scheduler, i.e. the child scheduler of base. */
static struct {
  lithe_sched_t CACHE_LINE_ALIGNED *sched;
  atomic_t CACHE_LINE_ALIGNED ref_count;
  atomic_t CACHE_LINE_ALIGNED harts_needed;
} __root_sched = {NULL, ATOMIC_INITIALIZER(0), ATOMIC_INITIALIZER(0)};

#define root_sched (__root_sched.sched)
#define root_sched_ref_count (__root_sched.ref_count)
#define root_sched_harts_needed (__root_sched.harts_needed)

static __thread struct {
  /* The next context to run on this vcore when lithe_vcore_entry is called again
   * after a yield from another lithe context */
  lithe_context_t *next_context;

  /* The current scheduler to run any contexts / functions with */
  lithe_sched_t *current_sched;

  /* Space for an arbitrary pointer of data we may wish to pass between
   * functions without using arguments */
  void *vcore_data;

} lithe_tls = {NULL, NULL, NULL};
#define next_context     (lithe_tls.next_context)
#define current_sched    (lithe_tls.current_sched)
#define vcore_data       (lithe_tls.vcore_data)
#define current_context  ((lithe_context_t*)current_uthread)

void __attribute__((constructor)) lithe_lib_init()
{
  init_once_racy(return);

  /* Create a lithe context for the main thread to run in */
  lithe_context_t *context = &lithe_main_context;
  assert(context);

  /* Set the scheduler associated with the context to be the base scheduler */
  context->sched = &base_sched;

  /* Publish our sched_ops, overriding the defaults */
  sched_ops = &lithe_sched_ops;

  /* Handle syscall events. */
  /* These functions are declared in parlib for simulating async syscalls on linux */
  ev_handlers[EV_SYSCALL] = lithe_handle_syscall;

  /* Once we have set things up for the main context, initialize the uthread
   * library with that main context */
  uthread_lib_init(&context->uth);

  /* Fill in the main context stack info. */
  parlib_get_main_stack(&context->stack.bottom, &context->stack.size);

  /* Initialize vcore request/yield data structures */
  lithe_vcore_init();

  /* Now that the library is initialized, a TLS should be set up for this
   * context, so set some of it */
  uthread_set_tls_var(&context->uth, current_sched, &base_sched);
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
    atomic_add(&base_sched.harts, 1);
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

static void lithe_thread_paused(struct uthread *uthread)
{
  uthread_runnable(uthread);
}

static void lithe_thread_has_blocked(struct uthread *uthread, int flags)
{
}

static void lithe_blockon_sysc(struct uthread* uthread, void *sysc)
{
  /* Set things up so we can wake this context up later */
  ((struct syscall*)sysc)->u_data = uthread;

  /* Inform the scheduler of the block. */
  assert(current_sched);
  assert(current_sched->funcs);
  assert(current_sched->funcs->context_block);
  current_sched->funcs->context_block(current_sched, (lithe_context_t*)uthread);
}

static void lithe_handle_syscall(struct event_msg *ev_msg, unsigned int ev_type)
{
  struct syscall *sysc;
  assert(in_vcore_context());
  assert(ev_msg);

  /* Get the sysc from the message. */
  sysc = ev_msg->ev_arg3;
  assert(sysc);

  /* Extract the uthread from it. */
  struct uthread *uthread = (struct uthread*)sysc->u_data;
  assert(uthread);
  assert(uthread->sysc == sysc); /* set in uthread.c */
  uthread->sysc = 0; /* so we don't 'reblock' on this later */

  /* Make it runnable again. It will become schedulable in the proper scheduler
   * after this. */
  lithe_context_t *ctx = (lithe_context_t*)uthread;
  lithe_sched_t *sched = current_sched;
  current_sched = ctx->sched;
  uthread_runnable(uthread);
  current_sched = sched;
}

static void __lithe_hart_grant(lithe_sched_t *child, void (*unlock_func) (void *), void *lock);
static void base_hart_enter(lithe_sched_t *__this)
{
  // This function will never return.  Either we eventually yield the vcore
  // back to the system, or we grant it to a child scheduler.  When one of
  // these two things happens, we will break out of this infinite loop...
  while (1) {
    // We need to do a bunch of refcounting here to make sure that we are able
    // to access the root_sched before passing a hart to it.  Only if we can
    // access it and are able to up its hart count, do we even attempt to
    // transfer control to it.
    // We down the refcount only after the hart returns to the base sched in
    // base_hart_return (or just below, if we didn't actually grant the hart).
    if (atomic_add_not_zero(&root_sched_ref_count, 1)) {
      rmb(); // order operations below with the atomic_add() above

      assert(root_sched);
      size_t old_harts_granted = atomic_add(&root_sched->harts, 1);
      if (old_harts_granted + 1 <= atomic_read(&root_sched_harts_needed)) {
        // Finish up our accounting
        atomic_add(&__this->harts, -1);
        // Reset the hart tls to its original state
        memset(&lithe_tls, 0, sizeof(lithe_tls));
        // And grant the hart down
        __lithe_hart_grant(root_sched, NULL, NULL);
      }
      atomic_add(&root_sched->harts, -1);
      // Release the root_sched ref (we decided not to grant a hart down)
      atomic_add(&root_sched_ref_count, -1);
    }
    // If this returns, we go back to the top of the loop and attempt to grant
    // to the root scheduler again
    atomic_add(&__this->harts, -1);
    maybe_vcore_yield();
    atomic_add(&__this->harts, 1);
    handle_events();
    cpu_relax();
  }
}

static void base_hart_return(lithe_sched_t *__this, lithe_sched_t *child)
{
  atomic_add(&root_sched_ref_count, -1);
}

static void base_sched_entered(lithe_sched_t *__this)
{
  /* Do nothing, we don't need any bookkeeping in the base scheduler */
}

static void base_sched_exited(lithe_sched_t *__this)
{
  /* Do nothing, we don't need any bookkeeping in the base scheduler */
}

static void base_child_entered(lithe_sched_t *__this, lithe_sched_t *child)
{
  assert(root_sched == NULL);
  root_sched_ref_count = ATOMIC_INITIALIZER(2);
  root_sched_harts_needed = ATOMIC_INITIALIZER(1);
  root_sched = child;
}

static void base_child_exited(lithe_sched_t *__this, lithe_sched_t *child)
{
  assert(root_sched == child);
  atomic_add(&root_sched_ref_count, -2);
  while (root_sched_ref_count)
    cpu_relax();
  root_sched = NULL;
}

static void base_hart_request(lithe_sched_t *__this, lithe_sched_t *child, int h)
{
  assert(root_sched == child);

  __sync_fetch_and_add(&root_sched_harts_needed, h);
  if (h > 0)
    maybe_vcore_request(h);
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
  next_context = context;
}

static void base_context_exit(lithe_sched_t *__this, lithe_context_t *context)
{
  fatal("The base context should never be exiting!\n");
}

lithe_sched_t *lithe_sched_current()
{
  return current_sched;
}

static void __lithe_hart_grant(lithe_sched_t *child, void (*unlock_func) (void *), void *lock)
{
  current_sched = child;
  if(unlock_func != NULL)
    unlock_func(lock);
  vcore_reenter(__lithe_sched_reenter);
  fatal("lithe: returned from enter");
}

void lithe_hart_grant(lithe_sched_t *child, void (*unlock_func) (void *), void *lock)
{
  assert(child);
  assert(in_vcore_context());
  assert(child != &base_sched);

  lithe_sched_t *parent = current_sched;

  /* Leave parent, join child. */
  atomic_add(&parent->harts, -1);
  atomic_add(&child->harts, 1);

  /* Enter the child scheduler */
  __lithe_hart_grant(child, unlock_func, lock);
}

void lithe_hart_yield()
{
  assert(in_vcore_context());
  assert(current_sched);
  assert(current_sched != &base_sched);

  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

  /* Switch to parent scheduler and notify it of the hart return */
  current_sched = parent;
  assert(current_sched);
  assert(current_sched->funcs);
  assert(current_sched->funcs->hart_return);
  current_sched->funcs->hart_return(current_sched, child);

  /* Leave child, reenter on parent. */
  atomic_add(&child->harts, -1);
  atomic_add(&parent->harts, 1);

  vcore_reenter(__lithe_sched_reenter);
  fatal("lithe: returned from hart yield");
}

static void __lithe_sched_enter(uthread_t *uthread, void *__arg)
{
  assert(in_vcore_context());

  /* Unpack the arguments to this function */
  lithe_context_t *context = (lithe_context_t*)uthread;
  struct { 
    lithe_sched_t *parent;
    lithe_sched_t *child;
  } *arg = __arg;
  lithe_sched_t *parent = arg->parent;
  lithe_sched_t *child = arg->child;

  assert(context);
  assert(parent);
  assert(child);

  /* Officially grant the hart to the child */
  atomic_add(&parent->harts, -1);
  atomic_add(&child->harts, 1);

  /* Set the scheduler to the child in the context that was blocked */
  context->sched = child;
  uthread_set_tls_var(&context->uth, current_sched, child);

  /* Inform parent. */
  assert(parent->funcs->child_enter);
  parent->funcs->child_enter(parent, child);

  /* Update the value of current_sched on this core. */
  current_sched = child;

  /* Inform child. */
  assert(child->funcs->sched_enter);
  child->funcs->sched_enter(child);

  /* Returning from this function will bring us back to the vcore_entry point.
   * We set next_context to continue running the child context now that it has
   * been properly set up. */
  next_context = context;
}

void lithe_sched_enter(lithe_sched_t *child)
{
  assert(!in_vcore_context());
  assert(current_sched);
  lithe_sched_t *parent = current_sched;

  /* Make sure that the childs 'funcs' and 'main_context' have already been set
   * up properly */
  assert(child->funcs);
  assert(child->main_context);

  /* Set-up child scheduler */
  child->harts = ATOMIC_INITIALIZER(0);
  child->parent = parent;

  /* Set up a function to run in vcore context to inform the parent that the
   * child has taken over */
  struct { 
    lithe_sched_t *parent;
    lithe_sched_t *child;
  } arg = {parent, child};

  /* Hijack the current context and replace it with the child->main_context.
   * Store the original context in child->parent_context so we can restore it
   * on exit. */
  child->parent_context = current_context;
  hijack_current_uthread(&child->main_context->uth);

  /* Yield this context to vcore context to run the function we just set up. Once
   * we return from the yield we will be fully inside the child scheduler. */
  uthread_yield(true, __lithe_sched_enter, &arg);
}

void __lithe_sched_exit(uthread_t *uthread, void *arg)
{
  assert(in_vcore_context());

  /* Unpack the arguments to this function */
  lithe_context_t *context = (lithe_context_t*)uthread;
  struct { 
    lithe_sched_t *parent;
    lithe_sched_t *child;
  } *real_arg = arg;
  lithe_sched_t *parent = real_arg->parent;
  lithe_sched_t *child = real_arg->child;

  assert(context);
  assert(parent);
  assert(child);

  /* Update child's hart count to 0 */
  atomic_add(&child->harts, -1);
  atomic_add(&parent->harts, 1);

  /* Set the scheduler to the parent in the context that was blocked */
  context->sched = parent;
  uthread_set_tls_var(&context->uth, current_sched, parent);

  /* Inform the child. */
  assert(child->funcs->sched_exit);
  child->funcs->sched_exit(child);

  /* Update the value of current_sched on this core. */
  current_sched = parent;

  /* Inform the parent that the child scheduler has exited. */
  assert(parent->funcs->child_exit);
  parent->funcs->child_exit(parent, child);

  /* Return to the original parent context */
  next_context = context;
}

void lithe_sched_exit()
{
  assert(!in_vcore_context());
  assert(current_sched);

  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

  /* Set ourselves up to jump to vcore context and run __lithe_sched_exit() */
  struct { 
    lithe_sched_t *parent;
    lithe_sched_t *child;
  } arg = {parent, child};

  /* Hijack the current context and replace it with the original parent context */
  hijack_current_uthread(&child->parent_context->uth);

  /* Yield this context to vcore context to run the function we just set up. Once
   * we return from the yield we will be fully back in the parent scheduler
   * running the original parent context. */
  uthread_yield(true, __lithe_sched_exit, &arg);

  /* Don't actually allow this context to continue until all of the child's
   * harts have been yielded. This field is synchronized with an update to its
   * value in lithe_hart_grant() as protected by a parent-scheduler-specific
   * locking function. 
   * Also, do a full blown lithe context yield so that this hart can do useful
   * work while waiting */
  long n;
  while ((n = atomic_read(&child->harts)) != 0) {
    assert(n >= 0);
    lithe_context_yield();
  }
}

void lithe_hart_request(int h)
{
  assert(current_sched);
  lithe_sched_t *parent = current_sched->parent;
  lithe_sched_t *child = current_sched;

  current_sched = parent;
  assert(parent->funcs->hart_request);
  parent->funcs->hart_request(parent, child, h);
  current_sched = child;
}

static void __lithe_context_yield(uthread_t *uthread, void *arg)
{
  assert(uthread);
  assert(current_sched);
  assert(current_sched->funcs);
  assert(in_vcore_context());

  lithe_context_t *context = (lithe_context_t*)uthread;
  assert(current_sched->funcs->context_yield);
  current_sched->funcs->context_yield(current_sched, context);
}

static void __lithe_context_finished(uthread_t *uthread, void *__arg)
{
  assert(uthread);
  assert(current_sched);
  assert(current_sched->funcs);
  assert(in_vcore_context());

  lithe_context_t *context = (lithe_context_t*)uthread;
  assert(current_sched->funcs->context_exit);
  current_sched->funcs->context_exit(current_sched, context);
}

static void __lithe_context_start()
{
  assert(current_context);
  assert(current_context->start_func);
  current_context->start_func(current_context->start_func_arg);

  uthread_yield(false, __lithe_context_finished, NULL);
}

static inline void __lithe_context_fields_init(lithe_context_t *context, lithe_sched_t *sched)
{
  context->start_func = NULL;
  context->start_func_arg = NULL;
  context->sched = sched;
  uthread_set_tls_var(&context->uth, current_sched, sched);
}

static inline void __lithe_context_reinit(lithe_context_t *context, lithe_sched_t *sched)
{
  /* Renitialize the new context as a uthread */
  uthread_init(&context->uth);

  /* Assign the context a new id */
  context->id = next_context_id++;

  /* Initialize the fields associated with a lithe context */
  __lithe_context_fields_init(context, sched);
}

static inline void __lithe_context_set_entry(lithe_context_t *context, 
                                             void (*func) (void *), void *arg)
{
  assert(context->stack.bottom);
  assert(context->stack.size);

  context->start_func = func;
  context->start_func_arg = arg;
  init_uthread_tf(&context->uth, __lithe_context_start, 
                  context->stack.bottom, context->stack.size);
}

void lithe_context_init(lithe_context_t *context, void (*func) (void *), void *arg)
{
  /* Assert that this is a valid context */
  assert(context);

  /* Zero out the uthread struct before passing it down: required by
   * uthread_init */
  memset(&context->uth, 0, sizeof(uthread_t));
  __lithe_context_reinit(context, current_sched);

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

void lithe_context_recycle(lithe_context_t *context, void (*func) (void *), void *arg)
{
  /* Assert that this is a valid context */
  assert(context);

  /* Initialize the fields associated with a lithe context */
  __lithe_context_fields_init(context, current_sched);

  /* Initialize context start function and stack */
  __lithe_context_set_entry(context, func, arg);
}

void lithe_context_reassociate(lithe_context_t *context, lithe_sched_t *sched)
{
  context->sched = sched;
  uthread_set_tls_var(&context->uth, current_sched, sched);
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

void __lithe_context_block(uthread_t *uthread, void *__arg)
{
  assert(__arg);
  assert(in_vcore_context());

  struct { 
    void (*func) (lithe_context_t *, void *); 
    void *arg;
    atomic_t safe_to_unblock;
  } *arg = __arg;

  /* Inform the scheduler of the block first. */
  assert(current_sched);
  assert(current_sched->funcs);
  assert(current_sched->funcs->context_block);
  current_sched->funcs->context_block(current_sched, (lithe_context_t*)uthread);

  /* Then carry out the call-site specific callback to do the blocking. */
  if (arg->func)
    arg->func((lithe_context_t*)uthread, arg->arg);

  atomic_set(&arg->safe_to_unblock, 1);
}
 
void lithe_context_block(void (*func)(lithe_context_t *, void *), void *arg)
{
  assert(!in_vcore_context());
  assert(current_context);

  struct { 
    void (*func) (lithe_context_t *, void *); 
    void *arg;
    atomic_t safe_to_unblock;
  } __arg = {func, arg, 0};
  uthread_yield(true, __lithe_context_block, &__arg);
  while(atomic_read(&__arg.safe_to_unblock) == 0);
}

void lithe_context_unblock(lithe_context_t *context)
{
  assert(context);
  uthread_runnable(&context->uth);
}

void lithe_context_yield()
{
  assert(!in_vcore_context());
  assert(current_sched);
  assert(current_context);

  uthread_yield(true, __lithe_context_yield, NULL);
}

void lithe_context_exit()
{
  assert(!in_vcore_context());
  assert(current_sched);
  assert(current_context);

  uthread_yield(false, __lithe_context_finished, NULL);
}

