/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef _VCORE_H
#define _VCORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ht/ht.h>
#include <ht/tls.h>
#include <sys/param.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define LOG2_MAX_VCORES LOG2_MAX_HTS
#define MAX_VCORES (1 << LOG2_MAX_VCORES)

/**
 * User defined callback function signalling that the vcore libary is done
 * initializing itself.  This function runs before main is called, and 
 * can, therefore be used for initialization of libraries that depend on 
 * using vcores.
 */
extern void vcore_ready();

extern int vcore_request(size_t k);
extern void vcore_yield(void);
extern void enable_notifs(uint32_t vcoreid);
extern void clear_notif_pending(uint32_t vcoreid);
static inline size_t max_vcores(void);
static inline size_t num_vcores(void);
static inline int vcore_id(void);
static inline bool in_vcore_context(void);

/* Static inlines */
static inline size_t max_vcores(void)
{
	return MIN(ht_limit_hard_threads(), MAX_VCORES);
}

static inline size_t num_vcores(void)
{
	return ht_num_hard_threads();
}

static inline int vcore_id(void)
{
	return ht_id();
}

static inline bool in_vcore_context(void)
{
	return in_ht_context();
}

static __inline void
init_user_context(struct ucontext *uc, uint32_t entry_pt, uint32_t stack_top)
{
    int ret = getcontext(uc);
	assert(ret == 0);
	uc->uc_stack.ss_sp = (void*)stack_top;
	makecontext(uc, (void*)entry_pt, 0);
}

#ifdef __cplusplus
}
#endif

#endif
