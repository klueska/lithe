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

/*
 * User defined entry point for a vcore. Called as a result of a
 * vcore_request()
 */
extern void vcore_entry();

extern int vcore_lib_init();
extern int vcore_request(size_t k);
extern void vcore_yield(bool preempt_pending);
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

#define vcore_set_tls_var(name, val)                                   \
{                                                                      \
	typeof(val) __val = val;                                           \
	begin_access_tls_vars(ht_tls_descs[vcore_id()]);                   \
	name = __val;                                                      \
	end_access_tls_vars();                                             \
}

#define vcore_get_tls_var(name)                                        \
({                                                                     \
	typeof(name) val;                                                  \
	begin_access_tls_vars(ht_tls_descs[vcore_id()]);                   \
	val = name;                                                        \
	end_access_tls_vars();                                             \
	val;                                                               \
})

#ifdef __cplusplus
}
#endif

#endif
