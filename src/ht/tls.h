/*
 * Copyright (c) 2011 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef HT_TLS_H
#define HT_TLS_H

#include <stdint.h>

/* Reference to the main threads tls descriptor */
extern void *main_tls_desc;

/* Current tls_desc for each running ht, used when swapping contexts onto an ht */
extern __thread void *current_tls_desc;

/* Get a TLS, returns 0 on failure.  Any thread created by a user-level
 * scheduler needs to create a TLS. */
void *allocate_tls(void);

/* Free a previously allocated TLS region */
void free_tls(void *tcb);

/* Passing in the htid, since it'll be in TLS of the caller */
void set_tls_desc(void *tls_desc, uint32_t htid);

/* Get the tls descriptor currently set for a given hard thread. This should
 * only ever be called once the hard thread has been initialized */
void *get_tls_desc(uint32_t htid);

#define safe_set_tls_var(name, val)                                     \
  void __attribute__((noinline, optimize("O0")))                        \
  safe_set_tls_var_internal() {                                         \
	asm("");                                                            \
    name = val;                                                         \
  } safe_set_tls_var_internal();                                        \

#define safe_get_tls_var(name)                                          \
({                                                                      \
  typeof(name) __attribute__((noinline, optimize("O0")))                \
  safe_get_tls_var_internal() {                                         \
	asm("");                                                            \
    return name;                                                        \
  } safe_get_tls_var_internal();                                        \
})

#endif /* HT_TLS_H */

