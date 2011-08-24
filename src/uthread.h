/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef _UTHREAD_H
#define _UTHREAD_H

#include <arch.h>
#include <vcore.h>

/* Bare necessities of a user thread.  2LSs should allocate a bigger struct and
 * cast their threads to uthreads when talking with vcore code.  Vcore/default
 * 2LS code won't touch udata or beyond. */
/* The definition of uthread_context_t is system dependant and located under
 * the proper arch.h file */
struct uthread {
	uthread_context_t uc;
	void *tls_desc;
	/* whether or not the scheduler can migrate you from your vcore */
	bool dont_migrate;
};
typedef struct uthread uthread_t;

/* External reference to the current uthread running on this vcore */
extern __thread uthread_t *current_uthread;

/* 2L-Scheduler operations.  Can be 0.  Examples in pthread.c. */
struct schedule_ops {
	/* Functions supporting thread ops */
	struct uthread *(*sched_init)(void);
	void (*sched_entry)(void);
	void (*thread_runnable)(struct uthread *);
	void (*thread_yield)(struct uthread *);
	/* Functions event handling wants */
	void (*preempt_pending)(void);
	void (*spawn_thread)(uintptr_t pc_start, void *data);	/* don't run yet */
};
extern struct schedule_ops *sched_ops;

/* Functions to make/manage uthreads.  Can be called by functions such as
 * pthread_create(), which can wrap these with their own stuff (like attrs,
 * retvals, etc). */

/* Entry point for a uthread.  Defined by the scheduler using the uthread
 * library */
extern void uthread_vcore_entry();

/* Initilization function for the uthread library */
int uthread_lib_init();

/* Initializes a uthread. */
void uthread_init(struct uthread *uth);

/* Reinitializes an already initialized uthread. */
void uthread_reinit(struct uthread *uth);

/* Cleans up a uthread that was previously initialized by a call to
 * uthread_init(). Be careful not to call this on any currently running
 * uthreads. */
void uthread_cleanup(struct uthread *uthread);

/* Function forcing a uthread to become runnable */
void uthread_runnable(struct uthread *uthread);

/* Function to yield a uthread - it can be made runnable again in the future */
void uthread_yield(bool save_state);

/* Utility function.  Event code also calls this. */
bool check_preempt_pending(uint32_t vcoreid);

/* Helpers, which sched_entry() can call */
void save_current_uthread(struct uthread *uthread);
void set_current_uthread(struct uthread *uthread);
void run_current_uthread(void) __attribute((noreturn));
void run_uthread(struct uthread *uthread) __attribute((noreturn));
void swap_uthreads(struct uthread *__old, struct uthread *__new);

static inline void
init_uthread_stack(uthread_t *uth, void *stack_top, uint32_t size)
{
  init_uthread_stack_ARCH(uth, stack_top, size);
}

static inline void
init_uthread_entry(uthread_t *uth, void (*entry)(void))
{
	init_uthread_entry_ARCH(uth, entry);
}

#define uthread_set_tls_var(uthread, name, val)                   \
{                                                                 \
	int vcoreid = vcore_id();                                     \
	volatile typeof(name) temp_val = (val);                       \
	void *temp_tls_desc = current_tls_desc;                       \
	wrfence();                                                    \
	set_tls_desc(((uthread_t*)(uthread))->tls_desc, vcoreid);     \
	name = temp_val;                                              \
    wrfence();                                                    \
	set_tls_desc(temp_tls_desc, vcoreid);                         \
    wrfence();                                                    \
}

#define uthread_get_tls_var(uthread, name)                        \
({                                                                \
	int vcoreid = vcore_id();                                     \
	typeof(name) val;                                             \
	void *temp_tls_desc = current_tls_desc;                       \
	set_tls_desc(((uthread_t*)(uthread))->tls_desc, vcoreid);     \
    wrfence();                                                    \
	val = name;                                                   \
	set_tls_desc(temp_tls_desc, vcoreid);                         \
    wrfence();                                                    \
	val;                                                          \
})

#endif /* _UTHREAD_H */
