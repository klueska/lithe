#include <sys/queue.h>
#include <parlib/slab.h>
#include <parlib/mcs.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include "lithe.h"
#include "futex.h"

struct futex_element {
  TAILQ_ENTRY(futex_element) link;
  lithe_context_t *context;
  mcs_lock_qnode_t *qnode;
  int *uaddr;
};
TAILQ_HEAD(futex_queue, futex_element);

struct futex_data {
  mcs_lock_t lock;
  struct futex_queue queue;
  struct kmem_cache *element_cache;
};
static struct futex_data __futex;

static inline void futex_init()
{
  mcs_lock_init(&__futex.lock);
  TAILQ_INIT(&__futex.queue);
  __futex.element_cache = kmem_cache_create("futex_element_cache", 
    sizeof(struct futex_element), __alignof__(struct futex_element),
    0, NULL, NULL);
}

static void __futex_block(lithe_context_t *context, void *arg) {
  struct futex_element *e = (struct futex_element*)arg;
  TAILQ_INSERT_TAIL(&__futex.queue, e, link);
  mcs_lock_unlock(&__futex.lock, e->qnode);
}

static inline int futex_wait(int *uaddr, int val)
{
  mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
  mcs_lock_lock(&__futex.lock, &qnode);
  if(*uaddr == val) {
    // We unlock in the body of __futex_block
    struct futex_element *e = kmem_cache_alloc(__futex.element_cache, 0); 
    e->uaddr = uaddr;
    e->qnode = &qnode;
    lithe_context_yield(__futex_block, e);
  }
  else {
    mcs_lock_unlock(&__futex.lock, &qnode);
  }
  return 0;
}

static inline int futex_wake(int *uaddr, int count)
{
  struct futex_element *e,*n = NULL;
  mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
  mcs_lock_lock(&__futex.lock, &qnode);
  e = TAILQ_FIRST(&__futex.queue);
  while(e != NULL) {
    if(count > 0) {
      n = TAILQ_NEXT(e, link);
      if(e->uaddr == uaddr) {
        TAILQ_REMOVE(&__futex.queue, e, link);
        lithe_context_unblock(e->context);
        kmem_cache_free(__futex.element_cache, e); 
        count--;
      }
      e = n;
    }
    else break;
  }
  mcs_lock_unlock(&__futex.lock, &qnode);
  return 0;
}

int futex(int *uaddr, int op, int val, const struct timespec *timeout,
                 int *uaddr2, int val3)
{
  assert(timeout == NULL);
  assert(uaddr2 == NULL);
  assert(val3 == 0);

  run_once(futex_init());
  switch(op) {
    case FUTEX_WAIT:
      return futex_wait(uaddr, val);
    case FUTEX_WAKE:
      return futex_wake(uaddr, val);
    default:
      errno = ENOSYS;
      return -1;
  }
  return -1;
}

