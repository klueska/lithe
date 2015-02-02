/* Copyright (c) 2014 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * Andrew Waterman <waterman@cs.berkeley.edu>
 * See COPYING for details.
 */

#include <sys/mman.h>
#include <parlib/waitfreelist.h>
#include "fork_join_sched.h"
#include "lithe.h"
#include "assert.h"

struct wfl sched_zombie_list = WFL_INITIALIZER(sched_zombie_list);
struct wfl context_zombie_list = WFL_INITIALIZER(context_zombie_list);

const lithe_sched_funcs_t lithe_fork_join_sched_funcs = {
  .hart_request    = lithe_fork_join_sched_hart_request,
  .hart_enter      = lithe_fork_join_sched_hart_enter,
  .hart_return     = lithe_fork_join_sched_hart_return,
  .sched_enter     = lithe_fork_join_sched_sched_enter,
  .sched_exit      = lithe_fork_join_sched_sched_exit,
  .child_enter     = lithe_fork_join_sched_child_enter,
  .child_exit      = lithe_fork_join_sched_child_exit,
  .context_block   = lithe_fork_join_sched_context_block,
  .context_unblock = lithe_fork_join_sched_context_unblock,
  .context_yield   = lithe_fork_join_sched_context_yield,
  .context_exit    = lithe_fork_join_sched_context_exit
};

static lithe_fork_join_context_t *__ctx_alloc(size_t stacksize)
{
    lithe_fork_join_context_t *ctx = wfl_remove(&context_zombie_list);
    if (!ctx) {
		int offset = rand_r(&rseed(0)) % max_vcores() * ARCH_CL_SIZE;
		stacksize += sizeof(lithe_fork_join_context_t) + offset;
		stacksize = ROUNDUP(stacksize, PGSIZE);
		void *stackbot = mmap(
			0, stacksize, PROT_READ|PROT_WRITE|PROT_EXEC,
			MAP_PRIVATE|MAP_ANONYMOUS, -1, 0
		);
		if (stackbot == MAP_FAILED)
			abort();
		ctx = stackbot + stacksize - sizeof(lithe_fork_join_context_t) - offset;
		ctx->context.stack.bottom = stackbot;
		ctx->context.stack.size = stacksize - sizeof(*ctx) - offset;
	}
	return ctx;
}

static void __ctx_free(lithe_fork_join_context_t *ctx)
{
    if (wfl_size(&context_zombie_list) < 1000) {
        wfl_insert(&context_zombie_list, ctx);
    } else {
		assert(!munmap(ctx->context.stack.bottom, ctx->context.stack.size));
	}
}

static int __thread_enqueue(lithe_fork_join_context_t *ctx, bool athead)
{
	ctx->state = FJS_CTX_RUNNABLE;

	int vcoreid = ctx->preferred_vcq;
	spin_pdr_lock(&tqlock(vcoreid));
	if (athead)
		TAILQ_INSERT_HEAD(&tqueue(vcoreid), &ctx->context, link);
	else
		TAILQ_INSERT_TAIL(&tqueue(vcoreid), &ctx->context, link);
	tqsize(vcoreid)++;
	spin_pdr_unlock(&tqlock(vcoreid));

	return vcoreid;
}

static lithe_fork_join_context_t *__thread_dequeue()
{
	inline lithe_fork_join_context_t *tdequeue(int vcoreid)
	{
		lithe_fork_join_context_t *ctx = NULL;
		if (tqsize(vcoreid)) {
			spin_pdr_lock(&tqlock(vcoreid));
			ctx = (lithe_fork_join_context_t*) TAILQ_FIRST(&tqueue(vcoreid));
			if (ctx) {
				TAILQ_REMOVE(&tqueue(vcoreid), &ctx->context, link);
				tqsize(vcoreid)--;
			}
			spin_pdr_unlock(&tqlock(vcoreid));
		}
		if (ctx)
			ctx->preferred_vcq = vcore_id();
		return ctx;
	}

	int vcoreid = vcore_id();
	lithe_fork_join_context_t *ctx = NULL;

	/* Try and grab a thread from our queue */
	ctx = tdequeue(vcoreid);

	/* If there isn't one, try and steal one from someone else's queue. */
	if (!ctx) {

		/* Steal up to half of the threads in the queue and return the first */
		lithe_fork_join_context_t *steal_threads(int vcoreid)
		{
			lithe_fork_join_context_t *ctx = NULL;
			int num_to_steal = (tqsize(vcoreid) + 1) / 2;
			if (num_to_steal) {
				ctx = tdequeue(vcoreid);
				if (ctx) {
					for (int i=1; i<num_to_steal; i++) {
						lithe_fork_join_context_t *u = tdequeue(vcoreid);
						if (u) __thread_enqueue(u, false);
						else break;
					}
				}
			}
			return ctx;
		}

		/* First try doing power of two choices. */
		int choice[2] = { rand_r(&rseed(vcoreid)) % num_vcores(),
		                  rand_r(&rseed(vcoreid)) % num_vcores()};
		int size[2] = { tqsize(choice[0]),
		                tqsize(choice[1])};
		int id = (size[0] > size[1]) ? 0 : 1;
		if (vcoreid != choice[id])
			ctx = steal_threads(choice[id]);
		else
			ctx = steal_threads(choice[!id]);

		/* Fall back to looping through all vcores. This time I go through
		 * max_vcores() just to make sure I don't miss anything. */
		if (!ctx) {
			int i = (vcoreid + 1) % max_vcores();
			while(i != vcoreid) {
				ctx = steal_threads(i);
				if (ctx) break;
				i = (i + 1) % max_vcores();
			}
		}
	}
	return ctx;
}

lithe_fork_join_sched_t *lithe_fork_join_sched_create()
{
  /* Allocate all the scheduler data together. */
  struct sched_data {
    lithe_fork_join_sched_t sched;
    lithe_fork_join_context_t main_context;
    struct lithe_fork_join_vc_mgmt vc_mgmt[];
  };

  /* Use a zombie list to reuse old schedulers if available, otherwise, create
   * a new one. */
  struct sched_data *s = wfl_remove(&sched_zombie_list);
  if (!s) {
    s = parlib_aligned_alloc(PGSIZE,
            sizeof(*s) + sizeof(struct lithe_fork_join_vc_mgmt) * max_vcores());
    s->sched.vc_mgmt = &s->vc_mgmt[0];
    s->sched.sched.funcs = &lithe_fork_join_sched_funcs;
  }

  /* Initialize the scheduler. */
  lithe_fork_join_sched_init(&s->sched, &s->main_context);
  return &s->sched;
}

void lithe_fork_join_sched_destroy(lithe_fork_join_sched_t *sched)
{
  lithe_fork_join_sched_cleanup(sched);
  if (wfl_size(&sched_zombie_list) < 100)
    wfl_insert(&sched_zombie_list, sched);
  else
    free(sched);
}

void lithe_fork_join_sched_init(lithe_fork_join_sched_t *sched,
                                lithe_fork_join_context_t *main_context)
{
  for (int i=0; i < max_vcores(); i++) {
    TAILQ_INIT(&tqueue_s(sched, i));
    spin_pdr_init(&tqlock_s(sched, i));
    tqsize_s(sched, i) = 0;
    rseed_s(sched, i) = i;
  }

  memset(main_context, 0, sizeof(*main_context));
  main_context->state = FJS_CTX_RUNNING;
  sched->sched.main_context = &main_context->context;

  sched->num_contexts = 1;
  sched->putative_child_hart_requests = 0;
  sched->granting_harts = 0;
  wfl_init(&sched->child_hart_requests);
}

void lithe_fork_join_sched_cleanup(lithe_fork_join_sched_t *sched)
{
  wfl_cleanup(&sched->child_hart_requests);
}

lithe_fork_join_context_t*
  lithe_fork_join_context_create(lithe_fork_join_sched_t *sched,
                                 size_t stack_size,
                                 void (*start_routine)(void*),
                                 void *arg)
{
  lithe_fork_join_context_t *ctx = __ctx_alloc(stack_size);
  lithe_fork_join_context_init(sched, ctx, start_routine, arg);
  return ctx;
}

void lithe_fork_join_context_init(lithe_fork_join_sched_t *sched,
                                  lithe_fork_join_context_t *ctx,
                                  void (*start_routine)(void*),
                                  void *arg)
{
  ctx->state = FJS_CTX_CREATED;
  ctx->preferred_vcq = vcore_id();
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
  __thread_enqueue(ctx, false);
  lithe_hart_request(1);
}

void lithe_fork_join_context_cleanup(lithe_fork_join_context_t *context)
{
  lithe_context_cleanup(&context->context);
}

void lithe_fork_join_context_destroy(lithe_fork_join_context_t *context)
{
  lithe_fork_join_context_cleanup(context);
  __ctx_free(context);
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

  /* Never request more than max_harts from the system. */
  k = k < max_harts() ? k : max_harts();

  /* Incrementally request harts from our parent. */
  int count = 0;
  __sync_fetch_and_add(&sched->putative_child_hart_requests, 1);
  for (int i = 0; i < k; i++)
    wfl_insert(&sched->child_hart_requests, child);
  count += lithe_hart_request(k);
  __sync_fetch_and_add(&sched->putative_child_hart_requests, -1);

  /* Return the number of successful requests. */
  return count;
}

void lithe_fork_join_sched_sched_enter(lithe_sched_t *__this)
{
}

void lithe_fork_join_sched_sched_exit(lithe_sched_t *__this)
{
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
  lithe_fork_join_context_t *ctx = __thread_dequeue();
  if (ctx != NULL) {
    assert(ctx->state == FJS_CTX_RUNNABLE);
    ctx->state = FJS_CTX_RUNNING;
    lithe_context_run(&ctx->context);
  }

  /* Otherwise, yield. */
  lithe_hart_yield();
}

void lithe_fork_join_sched_context_block(lithe_sched_t *__this,
                                         lithe_context_t *c)
{
}

void lithe_fork_join_sched_context_unblock(lithe_sched_t *__this,
                                           lithe_context_t *c)
{
  __thread_enqueue((lithe_fork_join_context_t*)c, true);
  lithe_hart_request(1);
}

void lithe_fork_join_sched_context_yield(lithe_sched_t *__this,
                                         lithe_context_t *c)
{
  __thread_enqueue((lithe_fork_join_context_t*)c, false);
  lithe_hart_request(1);
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
