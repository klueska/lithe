/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

#ifndef LITHE_DEFAULTS_H
#define LITHE_DEFAULTS_H

#include <unistd.h>
#include "lithe.h"
#include "fatal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Helpful defaults not associated with any specific lithe callback function */
static lithe_context_t* __lithe_context_create_default(bool stack)
{
  lithe_context_t *context = (lithe_context_t*)malloc(sizeof(lithe_context_t));
  assert(context);

  if(stack) {
    context->stack.size = 4*getpagesize();
    context->stack.bottom = malloc(context->stack.size);
    assert(context->stack.bottom);
  }
  return context;
}

static void __lithe_context_destroy_default(lithe_context_t *context, bool stack)
{
  if(stack) {
    assert(context->stack.bottom);
    free(context->stack.bottom);
  }

  assert(context);
  free(context);
}

/* Helpful defaults for the lithe callback functions */
static int __hart_request_default(lithe_sched_t *__this, lithe_sched_t *child, int k)
{
  return lithe_hart_request(k);
}

static void __hart_entry_default(lithe_sched_t *__this)
{
  fatal((char*)"Should not be calling hart_entry()");
}

static void __hart_return_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  lithe_hart_yield();
}

static void __child_enter_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  // Do nothing special by default when a child is entered
}

static void __child_exit_default(lithe_sched_t *__this, lithe_sched_t *child)
{
  // Do nothing special by default when a child is exited
}

static void __context_block_default(lithe_sched_t *__this, lithe_context_t *context)
{
  // Do nothing special by default when a context is blocked
}

static void __context_unblock_default(lithe_sched_t *__this, lithe_context_t *context)
{
  fatal((char*)"Should not be calling context_unblock()");
}

static void __context_yield_default(lithe_sched_t *__this, lithe_context_t *context)
{
  // Do nothing special by default when a context is yielded
}

static void __context_exit_default(lithe_sched_t *__this, lithe_context_t *context)
{
  // Do nothing special by default when a context exits
}

#ifdef __cplusplus
}
#endif

#endif
