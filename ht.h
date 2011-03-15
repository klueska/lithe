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

#include "htmain.h"
#include <ucontext.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Current user context running on each hard thread, used when interrupting a
 * user context because of async I/O or signal handling. Hard Thread 0's
 * current_ht_context is initialized to the continuation of the main thread's
 * context the first time it's ht_entry() function is invoked.
 */
extern __thread ucontext_t *current_ucontext;

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


#ifdef __cplusplus
}
#endif


#endif // HT_H
