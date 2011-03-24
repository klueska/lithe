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

#define LOG2_MAX_VCORES 6
#define MAX_VCORES (1 << LOG2_MAX_VCORES)

#define TRANSITION_STACK_PAGES 2
#define TRANSITION_STACK_SIZE (TRANSITION_STACK_PAGES*PGSIZE)

extern void vcore_entry();

/* Vcore API functions */
static inline size_t max_vcores(void);
static inline size_t num_vcores(void);
static inline int vcore_id(void);
static inline bool in_vcore_context(void);
int vcore_request(size_t k);
void vcore_yield(void);
void clear_notif_pending(uint32_t vcoreid);
void enable_notifs(uint32_t vcoreid);

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

#include <stdio.h>
static __inline void
init_user_context(struct ucontext *uc, uint32_t entry_pt, uint32_t stack_top)
{
    int ret = getcontext(uc);
	assert(ret == 0);
	uc->uc_stack.ss_sp = (void*)stack_top;
	makecontext(uc, (void*)entry_pt, 0);
}

/* Only call this if you know what you are doing. */
static inline void __enable_notifs(uint32_t vcoreid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

static inline void disable_notifs(uint32_t vcoreid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

#ifdef __cplusplus
}
#endif

#endif
