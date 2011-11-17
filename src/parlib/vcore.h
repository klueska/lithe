/*
 * Copyright (c) 2011 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

/**
 * Vcore interface.
 */

#ifndef __GNUC__
# error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#ifndef __ELF__
# error "expecting ELF file format"
#endif

#ifndef VCORE_H
#define VCORE_H

#define LOG2_MAX_VCORES 6
#define MAX_VCORES (1 << LOG2_MAX_VCORES)

#include <stdio.h>
#include <parlib/tls.h>
#include <sys/param.h>
#include <string.h>
#include <assert.h>
#include <ucontext.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Array of vcores */
struct vcore;
extern struct vcore *__vcores;

/**
 *  Array of pointers to TLS descriptors for each vcore.
 */
extern void **vcore_tls_descs;

/**
 * Context associated with each vcore. Serves as the entry point to this vcore
 * whenever the vcore is first brough up, a usercontext yields on it, or a
 * signal / async I/O notification is to be handled.
 */
extern __thread ucontext_t vcore_context;

/**
 * Current user context running on each vcore, used when interrupting a
 * user context because of async I/O or signal handling. Hard Thread 0's
 * vcore_saved_ucontext is initialized to the continuation of the main thread's
 * context the first time it's vcore_entry() function is invoked.
 */
extern __thread ucontext_t *vcore_saved_ucontext;

/**
 * Current tls_desc of the user context running on each vcore, used when
 * interrupting a user context because of async I/O or signal handling. Hard
 * Thread 0's vcore_saved_tls_desc is initialized to the tls_desc of the main
 * thread's context the first time it's vcore_entry() function is invoked.
 */
extern __thread void *vcore_saved_tls_desc;

/**
 * User defined entry point for each vcore.  If vcore_saved_ucontext is
 * set, this function should most likely just restore it, otherwise, go on from
 * there.
 */
extern void vcore_entry() __attribute__((weak));

/**
 * Initialization routine for the hrd threads subsystem.  Starts the process of
 * allocating vcore wrappers pinning them to cores, etc.
 */
extern int vcore_lib_init();

/**
 * Requests k additional vcores. Returns -1 if the request is impossible.
 * Otherwise, blocks calling vcore until the request is granted and returns 0.
*/
int vcore_request(int k);

/**
 * Relinquishes the calling vcore.
*/
void vcore_yield();

/**
 * Returns the id of the calling vcore.
 */
static inline int vcore_id(void)
{
	extern __thread int __vcore_id;
	return __vcore_id;
}

/**
 * Returns the current number of vcores allocated.
 */
static inline size_t num_vcores(void)
{
	extern volatile int __num_vcores;
	return __num_vcores;
}

/**
 * Returns the maximum number of vcores requested.
 */
static inline size_t max_vcores(void)
{
	extern volatile int __max_vcores;
	return __max_vcores;
}

/**
 * Returns the limit of allocatable vcores.
 */
static inline size_t limit_vcores(void)
{
	extern volatile int __limit_vcores;
	return MIN(__limit_vcores, MAX_VCORES);
}

/**
 * Returns whether you are currently running in vcore context or not.
 */
static inline bool in_vcore_context() {
	extern __thread bool __in_vcore_context;
	return __in_vcore_context;
}

/**
 * Clears the flag for pending notifications
 */
void clear_notif_pending(uint32_t vcoreid);

/**
 * Enable Notifications
 */
void enable_notifs(uint32_t vcoreid);

/**
 * Disable Notifications
 */
void disable_notifs(uint32_t vcoreid);

#define vcore_set_tls_var(name, val)                                   \
{                                                                      \
	typeof(val) __val = val;                                           \
	begin_access_tls_vars(vcore_tls_descs[vcore_id()]);                \
	name = __val;                                                      \
	end_access_tls_vars();                                             \
}

#define vcore_get_tls_var(name)                                        \
({                                                                     \
	typeof(name) val;                                                  \
	begin_access_tls_vars(vcore_tls_descs[vcore_id()]);                \
	val = name;                                                        \
	end_access_tls_vars();                                             \
	val;                                                               \
})

#ifdef __cplusplus
}
#endif

#endif // HT_H
