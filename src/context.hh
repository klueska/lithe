/*
 * Lithe Contexts
 */

#ifndef LITHE_CONTEXT_HH
#define LITHE_CONTEXT_HH

#include "context.h"
#include <parlib/mcs.h>

namespace lithe {

class Context: public lithe_context {
public:
  Context();
  Context(size_t stack_size, void (*start_routine)(void*), void *arg);
  virtual ~Context();

  virtual void reinit(size_t stack_size, void (*start_routine)(void*),
                      void *arg);

private:
  void (*start_routine)(void *);
  void *arg;

  static void start_routine_wrapper(void *__arg);
  void init(size_t stack_size, void (*start_routine)(void*), void *arg);
};

template<typename T>
class ContextFactory {
public:
  ContextFactory(size_t max_size = size_t(0)-1)
  {
    mcs_lock_init(&queue_lock);
    TAILQ_INIT(&queue);
  }

  ~ContextFactory()
  {
    lithe_context_t *e;
    TAILQ_FOREACH_REVERSE(e, &queue, lithe_context_queue, link)
      delete e;
  }

  virtual T *create(size_t stack_size, void (*start_routine)(void*), void *arg)
  {
    Context *c = NULL;

    /* Try and pull a context from the queue and recycle it */
    mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
    mcs_lock_lock(&queue_lock, &qnode);
      if((c = (Context*)TAILQ_FIRST(&queue)) != NULL)
      {
        TAILQ_REMOVE(&queue, c, link);
        queue_size--;
      }
    mcs_lock_unlock(&queue_lock, &qnode);

    if (c == NULL)
      c = new Context(stack_size, start_routine, arg);
    else
      c->reinit(stack_size, start_routine, arg);

    return c;
  }

  virtual void destroy(T* c)
  {
    /* See if we can afford to recycle the context, or just delete it */
    if (queue_size == max_size) {
      delete c;
    }
    else {
      mcs_lock_qnode_t qnode = MCS_QNODE_INIT;
      mcs_lock_lock(&queue_lock, &qnode);
        TAILQ_INSERT_TAIL(&queue, c, link);
        queue_size++;
      mcs_lock_unlock(&queue_lock, &qnode);
    }
  }

private:
  size_t max_size;
  size_t queue_size;

  mcs_lock_t queue_lock;
  lithe_context_queue_t queue;
};

}

#endif
