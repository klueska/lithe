/* Copyright (c) 2014 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * Andrew Waterman <waterman@cs.berkeley.edu>
 * See COPYING for details.
 */

/*
 * Simple fork-join scheduler.
 */

#ifndef LITHE_FORK_JOIN_SCHED_H
#define LITHE_FORK_JOIN_SCHED_H

#include "sched.h"
#include "context.h"
#include <parlib/waitfreelist.h>
#include <parlib/spinlock.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lithe_sched_funcs_t lithe_fork_join_sched_funcs;

/* Stack stuff. */
#define FJS_STACK_PAGES 1024
#define FJS_STACK_SIZE (FJS_STACK_PAGES*PGSIZE)

/* FJS_CTX states. */
#define FJS_CTX_CREATED			1
#define FJS_CTX_RUNNABLE		2
#define FJS_CTX_RUNNING			3
#define FJS_CTX_BLOCKED			4

struct lithe_fork_join_vc_mgmt {
	struct lithe_context_queue tqueue;
	spin_pdr_lock_t tqlock;
	int tqsize;
	unsigned int rseed;
	bool vconline;
} __attribute__((aligned(ARCH_CL_SIZE)));
#define tqueue_s(sched, i)   (sched)->vc_mgmt[(i)].tqueue
#define tqlock_s(sched, i)   (sched)->vc_mgmt[(i)].tqlock
#define tqsize_s(sched, i)   (sched)->vc_mgmt[(i)].tqsize
#define rseed_s(sched, i)    (sched)->vc_mgmt[(i)].rseed
#define vconline_s(sched, i) (sched)->vc_mgmt[(i)].vconline
#define tqueue(i)   tqueue_s((lithe_fork_join_sched_t*)lithe_sched_current(), i)
#define tqlock(i)   tqlock_s((lithe_fork_join_sched_t*)lithe_sched_current(), i)
#define tqsize(i)   tqsize_s((lithe_fork_join_sched_t*)lithe_sched_current(), i)
#define rseed(i)    rseed_s((lithe_fork_join_sched_t*)lithe_sched_current(), i)
#define vconline(i) vconline_s((lithe_fork_join_sched_t*)lithe_sched_current(), i)

typedef struct {
  lithe_sched_t sched;
  size_t num_contexts;
  size_t granting_harts;
  size_t num_harts_needed;
  spin_pdr_lock_t hart_request_lock;
  volatile int next_queue_id;
  struct lithe_sched_queue child_sched_list;
  spin_pdr_lock_t child_sched_list_lock;
  struct lithe_fork_join_vc_mgmt *vc_mgmt;
} lithe_fork_join_sched_t;

typedef struct {
  lithe_context_t context;
  uint32_t state;
  int preferred_vcq;
  void (*start_routine)(void*);
  void *arg;
  int stack_offset;
} lithe_fork_join_context_t;


/* API to request harts and make sure they are tracked properly when
 * "inheriting" from the lithe_fork_join_sched.  You should call this instead
 * of calling lithe_hart_request() directly. */
void lithe_fork_join_hart_request_inc(lithe_fork_join_sched_t *sched, int h);

/* Scheduler creation, initialization, etc. for the lithe_fork_join_sched. */
lithe_fork_join_sched_t *lithe_fork_join_sched_create();
void lithe_fork_join_sched_init(lithe_fork_join_sched_t *sched,
                                lithe_fork_join_context_t *main_context);
void lithe_fork_join_sched_cleanup(lithe_fork_join_sched_t *sched);
void lithe_fork_join_sched_destroy(lithe_fork_join_sched_t *sched);

/* Context creation, initialization, etc. for the lithe_fork_join_sched. */
lithe_fork_join_context_t*
  lithe_fork_join_context_create(lithe_fork_join_sched_t *sched,
                                 size_t stack_size,
                                 void (*start_routine)(void*),
                                 void *arg);
void lithe_fork_join_context_init(lithe_fork_join_sched_t *sched,
                                  lithe_fork_join_context_t *ctx,
                                  void (*start_routine)(void*),
                                  void *arg);
void lithe_fork_join_context_cleanup(lithe_fork_join_context_t *context);
void lithe_fork_join_context_destroy(lithe_fork_join_context_t *context);
void lithe_fork_join_sched_join_one(lithe_fork_join_sched_t *sched);
void lithe_fork_join_sched_join_all(lithe_fork_join_sched_t *sched);

/* Callback implementations that can be used by schedulers that "inherit" from
 * the lithe_fork_join_sched. */
void lithe_fork_join_sched_hart_request(lithe_sched_t *__this,
                                        lithe_sched_t *child,
                                        size_t h);
void lithe_fork_join_sched_sched_enter(lithe_sched_t *__this);
void lithe_fork_join_sched_sched_exit(lithe_sched_t *__this);
void lithe_fork_join_sched_child_enter(lithe_sched_t *__this,
                                       lithe_sched_t *child);
void lithe_fork_join_sched_child_exit(lithe_sched_t *__this,
                                      lithe_sched_t *child);
void lithe_fork_join_sched_hart_return(lithe_sched_t *__this,
                                       lithe_sched_t *child);
void lithe_fork_join_sched_hart_enter(lithe_sched_t *__this);
void lithe_fork_join_sched_context_block(lithe_sched_t *__this,
                                         lithe_context_t *c);
void lithe_fork_join_sched_context_unblock(lithe_sched_t *__this,
                                           lithe_context_t *c);
void lithe_fork_join_sched_context_yield(lithe_sched_t *__this,
                                         lithe_context_t *c);
void lithe_fork_join_sched_context_exit(lithe_sched_t *__this,
                                        lithe_context_t *c);

#ifdef __cplusplus
}
#endif

#endif  // LITHE_FORK_JOIN_SCHED_H
