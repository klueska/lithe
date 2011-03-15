#ifndef HT_TLS_H
#define HT_TLS_H

#include <stdint.h>

/* Get a TLS, returns 0 on failure.  Any thread created by a user-level
 * scheduler needs to create a TLS. */
void *allocate_tls(void);

/* Free a previously allocated TLS region */
void free_tls(void *tcb);

/* Initialize tls for use by a ht */
void init_tls(uint32_t htid);

/* Passing in the htid, since it'll be in TLS of the caller */
void set_tls_desc(void *tls_desc, uint32_t htid);

/* Get the tls descriptor currently set for a given hard thread. This should
 * only ever be called once the hard thread has been initialized */
void *get_tls_desc(uint32_t htid);

#endif /* HT_TLS_H */

