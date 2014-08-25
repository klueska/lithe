/* Copyright (c) 2014 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * Andrew Waterman <waterman@cs.berkeley.edu>
 * See COPYING for details.
 */

#include "fork_join_sched.h"
#include "lithe.h"

const lithe_sched_funcs_t lithe_fork_join_sched_funcs = {
  .hart_request    = lithe_fork_join_sched_hart_request,
  .hart_enter      = lithe_fork_join_sched_hart_enter,
  .hart_return     = lithe_fork_join_sched_hart_return,
  .child_enter     = lithe_fork_join_sched_child_enter,
  .child_exit      = lithe_fork_join_sched_child_exit,
  .context_block   = lithe_fork_join_sched_context_block,
  .context_unblock = lithe_fork_join_sched_context_unblock,
  .context_yield   = lithe_fork_join_sched_context_yield,
  .context_exit    = lithe_fork_join_sched_context_exit
};

static void schedule_context(lithe_fork_join_sched_t *sched,
                             lithe_context_t *context)
{
  wfl_insert(&sched->context_list, context);
  if (lithe_hart_request(1) == 0)
    __sync_fetch_and_add(&sched->allocated_harts, 1);
}

lithe_fork_join_sched_t *lithe_fork_join_sched_create()
{
  lithe_fork_join_sched_t *sched = malloc(sizeof(lithe_fork_join_sched_t));
  if (sched == NULL)
    abort();
  sched->sched.funcs = &lithe_fork_join_sched_funcs;
  lithe_fork_join_sched_init(sched);
  return sched;
}

void lithe_fork_join_sched_destroy(lithe_fork_join_sched_t *sched)
{
  lithe_fork_join_sched_cleanup(sched);
  free(sched);
}

void lithe_fork_join_sched_init(lithe_fork_join_sched_t *sched)
{
  memset(&sched->main_context_storage, 0, sizeof(sched->main_context_storage));
  sched->sched.main_context = &sched->main_context_storage;

  sched->num_contexts = 1;
  sched->num_blocked_contexts = 0;
  sched->allocated_harts = 1;
  sched->putative_child_hart_requests = 0;
  sched->granting_harts = 0;
  wfl_init(&sched->context_list);
  wfl_init(&sched->child_hart_requests);
}

void lithe_fork_join_sched_cleanup(lithe_fork_join_sched_t *sched)
{
  wfl_destroy(&sched->context_list);
  wfl_destroy(&sched->child_hart_requests);
}

lithe_context_t *lithe_fork_join_context_create(lithe_fork_join_sched_t *sched,
                                                size_t stack_size,
                                                void (*start_routine)(void*),
                                                void *arg)
{
  typedef struct {
    lithe_context_t context;
    void (*start_routine)(void*);
    void *arg;
  } fj_context_t;

  fj_context_t *context = malloc(sizeof(fj_context_t));
  if (context == NULL)
    abort();
  context->context.stack.bottom = malloc(stack_size);
  if (context->context.stack.bottom == NULL)
    abort();
  context->context.stack.size = stack_size;
  context->start_routine = start_routine;
  context->arg = arg;

  void start_routine_wrapper(void *arg)
  {
    fj_context_t *self = arg;
    self->start_routine(self->arg);
    destroy_dtls();
  }

  lithe_context_init(&context->context, start_routine_wrapper, context);
  __sync_fetch_and_add(&sched->num_contexts, 1);
  schedule_context(sched, &context->context);
  return &context->context;
}

void lithe_fork_join_context_destroy(lithe_context_t *context)
{
  lithe_context_cleanup(context);
  free(context->stack.bottom);
  free(context);
}

static void maybe_unblock_main_context(lithe_fork_join_sched_t *sched)
{
  if(__sync_add_and_fetch(&sched->num_contexts, -1) == 0)
    lithe_context_unblock(sched->sched.main_context);
}

static void block_main_context(lithe_context_t *c, void *arg)
{
  maybe_unblock_main_context(arg);
}

void lithe_fork_join_sched_joinAll(lithe_fork_join_sched_t *sched)
{
  lithe_context_block(block_main_context, sched);
}

int lithe_fork_join_sched_hart_request(lithe_sched_t *__this,
                                       lithe_sched_t *child,
                                       size_t k)
{
  lithe_fork_join_sched_t *sched = (void *)__this;
  __sync_fetch_and_add(&sched->putative_child_hart_requests, k);
    int ret = lithe_hart_request(k);
    if (ret == 0) {
      for (size_t i = 0; i < k; i++)
        wfl_insert(&sched->child_hart_requests, child);
    }
  __sync_fetch_and_add(&sched->putative_child_hart_requests, -k);
  return ret;
}

void lithe_fork_join_sched_child_enter(lithe_sched_t *__this,
                                       lithe_sched_t *child)
{
}

void lithe_fork_join_sched_child_exit(lithe_sched_t *__this,
                                      lithe_sched_t *child)
{
  lithe_fork_join_sched_t *sched = (void *)__this;
  wfl_remove_all(&sched->child_hart_requests, child);
  while (sched->granting_harts)
    cpu_relax();
}

void lithe_fork_join_sched_hart_return(lithe_sched_t *__this,
                                       lithe_sched_t *child)
{
}

static void decrement(void *gh)
{
  __sync_fetch_and_add((size_t*)gh, -1);
}

void lithe_fork_join_sched_hart_enter(lithe_sched_t *__this)
{
  lithe_fork_join_sched_t *sched = (void *)__this;

  while (1) {
    __sync_fetch_and_add(&sched->granting_harts, 1);
    lithe_sched_t *s = (lithe_sched_t*)wfl_remove(&sched->child_hart_requests);
    if (s != NULL)
      lithe_hart_grant(s, decrement, &sched->granting_harts);
    decrement(&sched->granting_harts);
    if (sched->putative_child_hart_requests == 0)
      break;
    cpu_relax();
  }

  lithe_context_t *c = (lithe_context_t*)wfl_remove(&sched->context_list);

  /* If there are no contexts to run, we can safely yield this hart */
  if(c == NULL) {
    __sync_fetch_and_add(&sched->allocated_harts, -1);
    lithe_hart_yield();
  /* Otherwise, run the context that we found */
  } else {
    lithe_context_run(c);
  }
}

void lithe_fork_join_sched_context_block(lithe_sched_t *__this,
                                         lithe_context_t *c)
{
  lithe_fork_join_sched_t *sched = (void *)__this;
  __sync_fetch_and_add(&sched->num_blocked_contexts, 1);
}

void lithe_fork_join_sched_context_unblock(lithe_sched_t *__this,
                                           lithe_context_t *c)
{
  lithe_fork_join_sched_t *sched = (void *)__this;
  __sync_fetch_and_add(&sched->num_blocked_contexts, -1);
  schedule_context(sched, c);
}

void lithe_fork_join_sched_context_yield(lithe_sched_t *__this,
                                         lithe_context_t *c)
{
  lithe_fork_join_sched_t *sched = (void *)__this;
  schedule_context(sched, c);
}

void lithe_fork_join_sched_context_exit(lithe_sched_t *__this,
                                        lithe_context_t *c)
{
  lithe_fork_join_sched_t *sched = (void *)__this;
  if (c != sched->sched.main_context) {
    lithe_fork_join_context_destroy(c);
    maybe_unblock_main_context(sched);
  }
}
