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
  size_t harts_needed = wfl_size(&sched->context_list);
  harts_needed += wfl_size(&sched->child_hart_requests);
  lithe_hart_request(1);
}

lithe_fork_join_sched_t *lithe_fork_join_sched_create()
{
  struct {
    lithe_fork_join_sched_t sched;
    lithe_fork_join_context_t main_context;
  } *s = malloc(sizeof(*s));
  if (s == NULL)
    abort();
  s->sched.sched.funcs = &lithe_fork_join_sched_funcs;
  lithe_fork_join_sched_init(&s->sched, &s->main_context);
  return &s->sched;
}

void lithe_fork_join_sched_destroy(lithe_fork_join_sched_t *sched)
{
  lithe_fork_join_sched_cleanup(sched);
  free(sched);
}

void lithe_fork_join_sched_init(lithe_fork_join_sched_t *sched,
                                lithe_fork_join_context_t *main_context)
{
  memset(main_context, 0, sizeof(*main_context));
  sched->sched.main_context = &main_context->context;

  sched->num_contexts = 1;
  sched->num_blocked_contexts = 0;
  sched->putative_child_hart_requests = 0;
  sched->granting_harts = 0;
  wfl_init(&sched->context_list);
  wfl_init(&sched->child_hart_requests);
}

void lithe_fork_join_sched_cleanup(lithe_fork_join_sched_t *sched)
{
  wfl_cleanup(&sched->context_list);
  wfl_cleanup(&sched->child_hart_requests);
}

lithe_fork_join_context_t*
  lithe_fork_join_context_create(lithe_fork_join_sched_t *sched,
                                 size_t stack_size,
                                 void (*start_routine)(void*),
                                 void *arg)
{
  void *storage = malloc(sizeof(lithe_fork_join_context_t) + stack_size);
  if (storage == NULL)
    abort();
  lithe_fork_join_context_t *ctx = storage;
  ctx->context.stack.bottom = storage + sizeof(lithe_fork_join_context_t);
  ctx->context.stack.size = stack_size;
  lithe_fork_join_context_init(sched, ctx, start_routine, arg);
  return ctx;
}

void lithe_fork_join_context_init(lithe_fork_join_sched_t *sched,
                                  lithe_fork_join_context_t *ctx,
                                  void (*start_routine)(void*),
                                  void *arg)
{
  ctx->start_routine = start_routine;
  ctx->arg = arg;

  void start_routine_wrapper(void *arg)
  {
    lithe_fork_join_context_t *self = arg;
    self->start_routine(self->arg);
    destroy_dtls();
  }

  lithe_context_init(&ctx->context, start_routine_wrapper, ctx);
  __sync_fetch_and_add(&sched->num_contexts, 1);
  schedule_context(sched, &ctx->context);
}

void lithe_fork_join_context_cleanup(lithe_fork_join_context_t *context)
{
  lithe_context_cleanup(&context->context);
}

void lithe_fork_join_context_destroy(lithe_fork_join_context_t *context)
{
  lithe_fork_join_context_cleanup(context);
  free(context);
}

void lithe_fork_join_sched_join_one(lithe_fork_join_sched_t *sched)
{
  if(__sync_add_and_fetch(&sched->num_contexts, -1) == 0)
    lithe_context_unblock(sched->sched.main_context);
}

static void block_main_context(lithe_context_t *c, void *arg)
{
  lithe_fork_join_sched_join_one(arg);
}

void lithe_fork_join_sched_join_all(lithe_fork_join_sched_t *sched)
{
  lithe_context_block(block_main_context, sched);
}

int lithe_fork_join_sched_hart_request(lithe_sched_t *__this,
                                       lithe_sched_t *child,
                                       size_t k)
{
  lithe_fork_join_sched_t *sched = (void *)__this;

  /* Don't even bother with the request if we already have a bunch of
   * outstanding requests for this child. */
  if (wfl_size(&sched->child_hart_requests) + k > max_vcores())
    return -1;

  /* Otherwise, submit the request. */
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

  /* If I have any outstanding requests from my children, preferentially pass
   * this hart down to them. */
  do {
    __sync_fetch_and_add(&sched->granting_harts, 1);
    lithe_sched_t *s = (lithe_sched_t*)wfl_remove(&sched->child_hart_requests);
    if (s != NULL) {
      lithe_hart_grant(s, decrement, &sched->granting_harts);
    }
    decrement(&sched->granting_harts);
  } while (sched->putative_child_hart_requests) ;

  /* Otherwise, if I have any contexts to run, grab one and run it. */
  lithe_context_t *c = (lithe_context_t*)wfl_remove(&sched->context_list);
  if (c != NULL)
    lithe_context_run(c);

  /* Otherwise, yield. */
  lithe_hart_yield();
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
    lithe_fork_join_context_destroy((lithe_fork_join_context_t *)c);
    lithe_fork_join_sched_join_one(sched);
  }
}
