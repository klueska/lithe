/* Copyright (c) 2008 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * See COPYING for details.
 */

/*
 * Basic deque (double-ended queue) functionality.
 *
 * In order to provide some rudimentary support for type-checking,
 * (and thus find more bugs) you can instantiate an instance
 * of a deque by expanding the macros DECLARE_TYPED_DEQUE and
 * DEFINE_TYPED_DEQUE. For example, to create an int deque you could
 * include in your declarations:
 *
 *   DECLARE_TYPED_DEQUE(int, int);
 *
 * And then include in your definitions:
 *
 *   DEFINE_TYPED_DEQUE(int, int);
 *
 * The deques do not perform as efficiently as they could, and the
 * implementations should be revisited when it can be profiled and
 * improved.
 *
 * Note, the deques are NOT thread safe.
 *
 */

#ifndef DEQUE_H
#define DEQUE_H

#include <errno.h>


#define DECLARE_TYPED_DEQUE(name, type)                                  \
                                                                         \
struct name##_deque_link {                                               \
  type element;                                                          \
  struct name##_deque_link *next;                                        \
  struct name##_deque_link *prev;                                        \
};                                                                       \
                                                                         \
struct name##_deque {                                                    \
  int length;                                                            \
  struct name##_deque_link *head;                                        \
  struct name##_deque_link *tail;                                        \
};                                                                       \
                                                                         \
                                                                         \
/* on success returns 0;                                                 \
   on failure returns -1 and sets errno appropriately */                 \
int name##_deque_init(struct name##_deque *);                            \
                                                                         \
/* adds an element to the tail of the deque;                             \
   on sucesss returns 0;                                                 \
   on failure returns -1 and sets errno appropriately */                 \
int name##_deque_enqueue(struct name##_deque *, type);                   \
                                                                         \
/* removes an element from the head of the deque;                        \
   on sucesss returns 0;                                                 \
   on failure returns -1 and sets errno appropriately */                 \
int name##_deque_dequeue(struct name##_deque *, type *);                 \
                                                                         \
/* pushes an element on the head of the deque;                           \
   on sucesss returns 0;                                                 \
   on failure returns -1 and sets errno appropriately */                 \
int name##_deque_push(struct name##_deque *, type);                      \
                                                                         \
/* pops an element from the head of the deque (synonym for dequeue);     \
   on sucesss returns 0;                                                 \
   on failure returns -1 and sets errno appropriately */                 \
int name##_deque_pop(struct name##_deque *, type *);                     \
                                                                         \
/* returns the number of elements in the deque;                          \
   on sucesss returns a number >= 0;                                     \
   on failure returns -1 and sets errno appropriately */                 \
int name##_deque_length(struct name##_deque *);                          \


#define DEFINE_TYPED_DEQUE(name, type)                                   \
                                                                         \
int name##_deque_init(struct name##_deque *q)                            \
{                                                                        \
  /* error out if necessary */                                           \
  if (q == NULL) {                                                       \
    errno = EINVAL;                                                      \
    return -1;                                                           \
  }                                                                      \
                                                                         \
  /* initialize head and tail */                                         \
  q->head = NULL;                                                        \
  q->tail = NULL;                                                        \
                                                                         \
  /* initialize length */                                                \
  q->length = 0;                                                         \
                                                                         \
  return 0;                                                              \
}                                                                        \
                                                                         \
                                                                         \
int name##_deque_enqueue(struct name##_deque *q, type e)                 \
{                                                                        \
  /* sanity check(s) */                                                  \
  if (q == NULL) {                                                       \
    errno = EINVAL;                                                      \
    return -1;                                                           \
  }                                                                      \
                                                                         \
  /* allocate the new link */                                            \
  struct name##_deque_link *link = (struct name##_deque_link *)          \
    malloc(sizeof(struct name##_deque_link));                            \
                                                                         \
  /* error out if necessary */                                           \
  if (link == NULL) {                                                    \
    errno = ENOMEM;                                                      \
    return -1;                                                           \
  }                                                                      \
                                                                         \
  /* initialize the link */                                              \
  link->element = e;                                                     \
  link->next = NULL;                                                     \
  link->prev = q->tail;                                                  \
                                                                         \
  /* update the deque */                                                 \
  if (q->length++ == 0) {                                                \
    q->head = q->tail = link;                                            \
  } else {                                                               \
    q->tail->next = link;                                                \
    q->tail = link;                                                      \
  }                                                                      \
                                                                         \
  return 0;                                                              \
}                                                                        \
                                                                         \
                                                                         \
int name##_deque_dequeue(struct name##_deque *q, type *e)                \
{                                                                        \
  /* sanity check(s) */                                                  \
  if (q == NULL) {                                                       \
    errno = EINVAL;                                                      \
    return -1;                                                           \
  }                                                                      \
                                                                         \
  /* maybe not permitted */                                              \
  if (q->length == 0) {                                                  \
    errno = EPERM;                                                       \
    return -1;                                                           \
  }                                                                      \
                                                                         \
  /* retrieve element */                                                 \
  *e = q->head->element;                                                 \
                                                                         \
  /* save the current head for deallocation */                           \
  struct name##_deque_link *head;                                        \
  head = q->head;                                                        \
                                                                         \
  /* update the deque */                                                 \
  if (q->length-- == 1) {                                                \
    q->head = q->tail = NULL;                                            \
  } else {                                                               \
    q->head = q->head->next;                                             \
  }                                                                      \
                                                                         \
  /* deallocate old head */                                              \
  free(head);                                                            \
                                                                         \
  return 0;                                                              \
}                                                                        \
                                                                         \
                                                                         \
int name##_deque_push(struct name##_deque *q, type e)                    \
{                                                                        \
  /* sanity check(s) */                                                  \
  if (q == NULL) {                                                       \
    errno = EINVAL;                                                      \
    return -1;                                                           \
  }                                                                      \
                                                                         \
  /* allocate the new link */                                            \
  struct name##_deque_link *link = (struct name##_deque_link *)          \
    malloc(sizeof(struct name##_deque_link));                            \
                                                                         \
  /* error out if necessary */                                           \
  if (link == NULL) {                                                    \
    errno = ENOMEM;                                                      \
    return -1;                                                           \
  }                                                                      \
                                                                         \
  /* initialize the link */                                              \
  link->element = e;                                                     \
  link->next = q->head;                                                  \
  link->prev = NULL;                                                     \
                                                                         \
  /* update the deque */                                                 \
  if (q->length++ == 0) {                                                \
    q->head = q->tail = link;                                            \
  } else {                                                               \
    q->head->prev = link;                                                \
    q->head = link;                                                      \
  }                                                                      \
                                                                         \
  return 0;                                                              \
}                                                                        \
                                                                         \
                                                                         \
int name##_deque_pop(struct name##_deque *q, type *e)                    \
{                                                                        \
  return name##_deque_dequeue(q, e);                                     \
}                                                                        \
                                                                         \
                                                                         \
int name##_deque_length(struct name##_deque *q)                          \
{                                                                        \
  /* sanity check(s) */                                                  \
  if (q == NULL) {                                                       \
    errno = EINVAL;                                                      \
    return -1;                                                           \
  }                                                                      \
                                                                         \
  return q->length;                                                      \
}


#define DECLARE_DEFINE_TYPED_DEQUE(name, type)                           \
  DECLARE_TYPED_DEQUE(name, type);                                       \
  DEFINE_TYPED_DEQUE(name, type)


#endif // DEQUE_H
