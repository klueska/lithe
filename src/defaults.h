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
lithe_context_t* __lithe_context_create_default(bool stack);
void __lithe_context_destroy_default(lithe_context_t *context, bool stack);

/* Helpful defaults for the lithe callback functions */
void __hart_request_default(lithe_sched_t *__this, lithe_sched_t *child, size_t h);
void __hart_entry_default(lithe_sched_t *__this);
void __hart_return_default(lithe_sched_t *__this, lithe_sched_t *child);
void __sched_enter_default(lithe_sched_t *__this);
void __sched_exit_default(lithe_sched_t *__this);
void __child_enter_default(lithe_sched_t *__this, lithe_sched_t *child);
void __child_exit_default(lithe_sched_t *__this, lithe_sched_t *child);
void __context_block_default(lithe_sched_t *__this, lithe_context_t *context);
void __context_unblock_default(lithe_sched_t *__this, lithe_context_t *context);
void __context_yield_default(lithe_sched_t *__this, lithe_context_t *context);
void __context_exit_default(lithe_sched_t *__this, lithe_context_t *context);

#ifdef __cplusplus
}
#endif

#endif
