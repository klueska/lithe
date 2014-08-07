#include <sys/queue.h>
#include <parlib/waitfreelist.h>
#include "internal/assert.h"
#include <stdio.h>
#include <errno.h>
#include "lithe.h"
#include "futex.h"

struct futex_list;

struct futex_element {
  STAILQ_ENTRY(futex_element) next;
  lithe_context_t *context;
  struct futex_list *list;
  int val;
};

STAILQ_HEAD(futex_tailq, futex_element);

struct futex_list {
  struct futex_tailq tailq;
  int *uaddr;
  spinlock_t lock;
};

/* A list of futex blocking queues, one per uaddr. */
struct wfl futex_lists = WFL_INITIALIZER(futex_lists);
spinlock_t futex_lists_lock = SPINLOCK_INITIALIZER;

/* Find or create the blocking queue that corresponds to the uaddr. */
static struct futex_list *get_futex_list(int *uaddr)
{
  struct futex_list *list;
  wfl_foreach_unsafe(list, &futex_lists) {
    if (list->uaddr == uaddr)
      return list;
  }

  spinlock_lock(&futex_lists_lock);
    wfl_foreach_unsafe(list, &futex_lists) {
      if (list->uaddr == uaddr)
        break;
    }
    if (list == NULL) {
      list = malloc(sizeof(struct futex_list));
      if (list == NULL)
        abort();
      STAILQ_INIT(&list->tailq);
      list->uaddr = uaddr;
      spinlock_init(&list->lock);
      wfl_insert(&futex_lists, list);
    }
  spinlock_unlock(&futex_lists_lock);

  return list;
}

/* lithe_context_block callback.  Atomically checks uaddr == val and blocks. */
static void __futex_block(lithe_context_t *context, void *arg) {
  struct futex_element *e = arg;
  bool block = true;

  spinlock_lock(&e->list->lock);
    if (*e->list->uaddr == e->val) {
      e->context = context;
      STAILQ_INSERT_TAIL(&e->list->tailq, e, next);
    } else {
      block = false;
    }
  spinlock_unlock(&e->list->lock);

  if (!block)
    lithe_context_unblock(context);
}

int futex_wait(int *uaddr, int val)
{
  if (*uaddr == val) {
    struct futex_element e;
    e.list = get_futex_list(uaddr);
    e.val = val;
    lithe_context_block(__futex_block, &e);
  }
  return 0;
}

int futex_wake_one(int *uaddr)
{
  struct futex_list *list = get_futex_list(uaddr);
  spinlock_lock(&list->lock);
    struct futex_element *e = STAILQ_FIRST(&list->tailq);
    if (e != NULL)
      STAILQ_REMOVE_HEAD(&list->tailq, next);
  spinlock_unlock(&list->lock);

  if (e != NULL) {
    lithe_context_unblock(e->context);
    return 1;
  }
  return 0;
}

static inline int unblock_futex_queue(struct futex_tailq *q)
{
  int num = 0;
  struct futex_element *e,*n;
  for (e = STAILQ_FIRST(q), num = 0; e != NULL; e = n, num++) {
    n = STAILQ_NEXT(e, next);
    lithe_context_unblock(e->context);
  }

  return num;
}

int futex_wake_all(int *uaddr)
{
  struct futex_list *list = get_futex_list(uaddr);

  spinlock_lock(&list->lock);
    struct futex_tailq q = list->tailq;
    STAILQ_INIT(&list->tailq);
  spinlock_unlock(&list->lock);

  return unblock_futex_queue(&q);
}

int futex_wake_some(int *uaddr, int count)
{
  struct futex_tailq q = STAILQ_HEAD_INITIALIZER(q);
  struct futex_list *list = get_futex_list(uaddr);
  struct futex_element *e;

  spinlock_lock(&list->lock);
    /* Remove up to count entries from the queue, and remeber them locally. */
    while (count-- > 0 && (e = STAILQ_FIRST(&list->tailq)) != NULL) {
      STAILQ_REMOVE_HEAD(&list->tailq, next);
      STAILQ_INSERT_TAIL(&q, e, next);
    }
  spinlock_unlock(&list->lock);

  return unblock_futex_queue(&q);
}

int futex(int *uaddr, int op, int val, const struct timespec *timeout,
                 int *uaddr2, int val3)
{
  assert(timeout == NULL);
  assert(uaddr2 == NULL);
  assert(val3 == 0);

  switch(op) {
    case FUTEX_WAIT:
      return futex_wait(uaddr, val);
    case FUTEX_WAKE:
      return futex_wake_some(uaddr, val);
    default:
      errno = ENOSYS;
      return -1;
  }
  return -1;
}

