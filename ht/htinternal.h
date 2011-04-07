/*
 * Copyright (c) 2011 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef HT_INTERNAL_H
#define HT_INTERNAL_H

/* #include this file for all internal modules so they get access to the type
 * definitions and externed variables below */

#include <stdbool.h>
#include <pthread.h>
#include <asm/ldt.h>
#include <bits/local_lim.h>
#include <ht/arch.h>

struct hard_thread {
  bool created __attribute__((aligned (ARCH_CL_SIZE)));
  bool allocated;
  bool running __attribute__((aligned (ARCH_CL_SIZE)));
  struct user_desc ldt_entry;
  pthread_t thread;
};

/* Array of hard threads */
extern struct hard_thread *__ht_threads;
 
/* Id of the currently running hard thread. */
extern __thread int __ht_id;

/**
 * Pointer to the main thread's tls descriptor (needed for bootstrapping).
 */
extern void *__ht_main_tls_desc;

/* Now #include the externally exposed ht interface header so that internal
 * files only need to #include this header */
#include <ht/ht.h>

#endif // HT_INTERNAL_H
