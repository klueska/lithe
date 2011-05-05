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
#include <asm/ldt.h>
#include <sched.h>
#include <bits/local_lim.h>
#include <ht/arch.h>

//#define HT_MIN_STACK_SIZE (4*16384)
#define HT_MIN_STACK_SIZE (3*PGSIZE)

struct hard_thread {
  /* For bookkeeping */
  bool created __attribute__((aligned (ARCH_CL_SIZE)));
  bool allocated __attribute__((aligned (ARCH_CL_SIZE)));
  bool running __attribute__((aligned (ARCH_CL_SIZE)));
  bool yielding __attribute__((aligned (ARCH_CL_SIZE)));

  /* The ldt entry associated with this hard thread. Used for manging TLS in
   * user space. */
  struct user_desc ldt_entry;
  
  /* Thread properties when running in ht context: stack + TLS stuff */
  pid_t ptid;
  void *stack_top;
  size_t stack_size;
  void *tls_desc;
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
