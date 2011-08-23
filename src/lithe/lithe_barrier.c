#include "lithe_barrier.h"
#include <ht/atomic.h>
#include <stdlib.h>
#include <assert.h>


void lithe_barrier_init(lithe_barrier_t *barrier, int N) 
{
  assert(barrier != NULL);
  barrier->N = N;
  barrier->arrived = 0;
  barrier->wait = false;
  barrier->signals = (padded_bool_t *)calloc(N, sizeof(padded_bool_t));
  barrier->blocked[0].queue = (lithe_context_t **)malloc(N * sizeof(lithe_context_t *));
  barrier->blocked[1].queue = (lithe_context_t **)malloc(N * sizeof(lithe_context_t *));
  barrier->blocked[0].len = 0;
  barrier->blocked[1].len = 0;
  barrier->blocked[0].maxlen = N;
  barrier->blocked[1].maxlen = N;
  spinlock_init(&barrier->blocked[0].mtx);
  spinlock_init(&barrier->blocked[1].mtx);
}


void lithe_barrier_destroy(lithe_barrier_t *barrier) 
{
  assert(barrier != NULL);
  free(barrier->signals);
  free(barrier->blocked[0].queue);
  free(barrier->blocked[1].queue);
}


void __lithe_barrier_block(lithe_context_t *context, void *__blocked) 
{
  contextq_t *blocked = (contextq_t *)__blocked;
  assert(blocked != NULL);
  assert(blocked->len < blocked->maxlen);
  blocked->queue[blocked->len] = context;
  blocked->len += 1;
  spinlock_unlock(&blocked->mtx);
}


void lithe_barrier_wait(lithe_barrier_t *barrier) 
{
  assert(barrier != NULL);

  /* toggled signal value for barrier reuse */
  bool wait = coherent_read(barrier->wait);

  /* increment the counter to signal arrival */
  int id = fetch_and_add(&barrier->arrived, 1);

  /* if last to arrive at barrier, release everyone else */ 
  if (id == (barrier->N - 1)) {
    /* reset barrier */
    barrier->arrived = 0;
    rdfence();
    barrier->wait = !barrier->wait;
    wrfence();

    /* signal everyone that they can continue */
    int i;
    for (i = 0; i < barrier->N; i++) {
      barrier->signals[i].val = !wait;
    }

    /* unblock those that are no longer running */
    contextq_t *blocked = &barrier->blocked[wait];
    spinlock_lock(&blocked->mtx);
    for (i = 0; i < blocked->len; i++) {
      lithe_context_unblock(blocked->queue[i]);
    }
    blocked->len = 0;
    spinlock_unlock(&blocked->mtx);
  } 
  
  /* wait for remaining to arrive */
  else {
    /* spin for MAXSTALLS to wait for minor load imbalance */
    /* release hart afterwards to avoid deadlock */
    const int MAXSTALLS = 1000;
    int nstalls = 0;
    while (coherent_read(barrier->signals[id].val) == wait) { 
      nstalls++;
      if (nstalls >= MAXSTALLS) {
	contextq_t *blocked = &barrier->blocked[wait];
	spinlock_lock(&blocked->mtx);
	if (barrier->signals[id].val == wait) {
	  lithe_context_block(__lithe_barrier_block, (void *)blocked);
	} else {
	  spinlock_unlock(&blocked->mtx);
	}
      }
    }
  }
}


