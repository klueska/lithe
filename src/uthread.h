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
	struct uthread *(*thread_create)(void (*func)(void), void *);
	void (*thread_runnable)(struct uthread *);
	void (*thread_yield)(struct uthread *);
	void (*thread_exit)(struct uthread *);
	unsigned int (*vcores_wanted)(void);
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
int uthread_init();

/* Creates a uthread.  Will pass udata to sched_ops's thread_create.  Func is
 * what gets run, and if you want args, wrap it (like pthread) */
struct uthread *uthread_create(void (*func)(void), void *udata);

/* Destroys a uthread. Be careful not to call this on any currently running
 * threads */
void uthread_destroy(struct uthread *uthread);

/* Function forcing a uthread to become runnable */
void uthread_runnable(struct uthread *uthread);

/* Function to yield a uthread - it can be made runnable again in the future */
void uthread_yield(void);

/* Function to exit a uthread completely */
void uthread_exit(void);

/* Utility function.  Event code also calls this. */
bool check_preempt_pending(uint32_t vcoreid);

/* Helpers, which sched_entry() can call */
void save_current_uthread(struct uthread *uthread);
void run_current_uthread(void) __attribute((noreturn));
void run_uthread(struct uthread *uthread) __attribute((noreturn));
void swap_uthreads(struct uthread *old, struct uthread *new);

static inline void
init_uthread_stack(uthread_t *uth, void *stack_top, uint32_t size)
{
  init_uthread_stack_ARCH(uth, stack_top, size);
}

#define init_uthread_entry(uth, entry, argc, ...) \
	init_uthread_entry_ARCH((uth), entry, (argc), ##__VA_ARGS__);

#define uthread_set_tls_var(uthread, name, val)                   \
{                                                                 \
	int vcoreid = vcore_id();                                     \
	typeof(name) temp_val = (val);                                \
	void *temp_tls_desc = current_tls_desc;                       \
	set_tls_desc(((uthread_t*)(uthread))->tls_desc, vcoreid);     \
	name = temp_val;                                              \
	set_tls_desc(temp_tls_desc, vcoreid);                         \
}

#define uthread_get_tls_var(uthread, name)                        \
({                                                                \
	int vcoreid = vcore_id();                                     \
	typeof(name) val;                                             \
	void *temp_tls_desc = current_tls_desc;                       \
	set_tls_desc(((uthread_t*)(uthread))->tls_desc, vcoreid);     \
	val = name;                                                   \
	set_tls_desc(temp_tls_desc, vcoreid);                         \
	val;                                                          \
})

#endif /* _UTHREAD_H */
