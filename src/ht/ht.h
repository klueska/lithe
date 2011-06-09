/*
 * Copyright (c) 2011 The Regents of the University of California
 * Benjamin Hindman <benh@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

/**
 * Hard thread interface.
 */

#ifndef __GNUC__
# error "expecting __GNUC__ to be defined (are you using gcc?)"
#endif

#ifndef __ELF__
# error "expecting ELF file format"
#endif

#ifndef HT_H
#define HT_H

#define LOG2_MAX_HTS 6
#define MAX_HTS (1 << LOG2_MAX_HTS)

#include <ucontext.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wrapper around mutex types for locking an ht
 * Must be cast to real underlying type when used
 */
typedef void ht_mutex_t;

/**
 *  Array of pointers to TLS descriptors for each hard thread.
 */
extern void **ht_tls_descs;

/**
 * Context associated with each hard thread. Serves as the entry point to this
 * hard thread whenever the hard thread is first brough up, a usercontext
 * yields on it, or a signal / async I/O notification is to be handled.
 */
extern __thread ucontext_t ht_context;

/**
 * Current user context running on each hard thread, used when interrupting a
 * user context because of async I/O or signal handling. Hard Thread 0's
 * current_ht_context is initialized to the continuation of the main thread's
 * context the first time it's ht_entry() function is invoked.
 */
extern __thread ucontext_t *current_ucontext;

/* MCS lock required to be held when yielding an ht.  This variable stores a
 * reference to that value as it is passed in via a call to ht_yield */
extern pthread_mutex_t ht_yield_lock;

/**
 * Current TLS descriptor running on each hard thread, used when interrupting a
 * user context because of async I/O or signal handling. Hard Thread 0's
 * current_tls_desc is initialized to the TLS descriptor of the main thread's
 * context the first time it's ht_entry() function is invoked.
 */
extern __thread void *current_tls_desc;

/**
 * User defined callback function signalling that the ht libary is done
 * initializing itself.  This function runs before main is called, and 
 * can, therefore be used for initialization of libraries that depend on 
 * using hard threads. 
 */
extern void ht_ready();

/**
 * User defined entry point for each hard thread.  If current_user_context is
 * set, this function should most likely just restor it, otherwise, go on from
 * there.
 */
extern void ht_entry();

/**
 * Requests k additional hard threads. Returns -1 if the request is
 * impossible or ht_set_entrance has not been called. Otherwise,
 * blocks calling hard thread until the request is granted and returns
 * number of hard threads granted.
*/
int ht_request(int k);

/**
 * Requests k additional hard threads without blocking current hard
 * thread. Returns -1 if the request is impossible or ht_set_entrance
 * has not been called. If k is negative then the total number of hard
 * threads ...
 */
int ht_request_async(int k);

/**
 * Relinquishes the calling hard thread.
*/
void ht_yield();

/**
 * Returns the id of the calling hard thread.
 */
int ht_id();

/**
 * Returns the current number of hard threads allocated.
 */
int ht_num_hard_threads();

/**
 * Returns the maximum number of hard threads requested.
 */
int ht_max_hard_threads();

/**
 * Returns the limit of allocatable hard threads.
 */
int ht_limit_hard_threads();

/**
 * Returns whether you are currently running in ht context or not.
 */
static inline bool in_ht_context() {
	extern __thread bool __in_ht_context;
	return __in_ht_context;
}

/**
 * Wrapper for locking hard threads 
 */
int ht_lock(ht_mutex_t *mutex);

/** 
 * Wrapper for trylocking hard threads 
 */
int ht_trylock(ht_mutex_t *mutex);

/**
 * Wrapper for unlocking hard threads 
 */
int ht_unlock(ht_mutex_t *mutex);

/**
 * Clears the flag for pending notifications
 */
void clear_notif_pending(uint32_t htid);

/**
 * Enable Notifications
 */
void enable_notifs(uint32_t htid);

/**
 * Disable Notifications
 */
void disable_notifs(uint32_t htid);

#ifdef __cplusplus
}
#endif


#endif // HT_H
