/*
 * Copyright (c) 2011 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef VCORE_INTERNAL_H
#define VCORE_INTERNAL_H

/* #include this file for all internal modules so they get access to the type
 * definitions and externed variables below */

#include <stdbool.h>
#include <asm/ldt.h>
#include <bits/local_lim.h>
#include <parlib/arch.h>

#define VCORE_USE_PTHREAD
#ifdef VCORE_USE_PTHREAD
  #include <pthread.h>
#endif

#define VCORE_MIN_STACK_SIZE (3*PGSIZE)

struct vcore {
  /* For bookkeeping */
  bool created __attribute__((aligned (ARCH_CL_SIZE)));
  bool allocated __attribute__((aligned (ARCH_CL_SIZE)));
  bool running __attribute__((aligned (ARCH_CL_SIZE)));

  /* The ldt entry associated with this vcore. Used for manging TLS in
   * user space. */
  struct user_desc ldt_entry;
  
#ifdef VCORE_USE_PTHREAD
  /* The linux pthread associated with this vcore */
  pthread_t thread;
#else
  /* Thread properties when running in vcore context: stack + TLS stuff */
  pid_t ptid;
  void *stack_top;
  size_t stack_size;
  void *tls_desc;
#endif
};

/* Now #include the externally exposed vcore interface header so that internal
 * files only need to #include this header */
#include <parlib/vcore.h>

#endif // VCORE_INTERNAL_H
