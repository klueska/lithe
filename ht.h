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

#ifdef __cplusplus
extern "C" {
#endif

/* Used to request maximum number of allocatable hard threads. */
const int HT_REQUEST_MAX = -1;

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
#ifdef __PIC__
#define ht_yield()                                                    \
({                                                                    \
  asm volatile ("jmp __ht_yield@PLT");                                \
  -1;                                                                 \
})
#else
#define ht_yield()                                                    \
({                                                                    \
  asm volatile ("jmp __ht_yield");                                    \
  -1;                                                                 \
})
#endif /* __PIC__ */


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
