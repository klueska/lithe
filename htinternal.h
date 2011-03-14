#ifndef HT_INTERNAL_H
#define HT_INTERNAL_H

/* #include this file for all internal modules so they get access to the type
 * definitions and externed variables below */

#include <arch.h>
#include <stdbool.h>
#include <pthread.h>
#include <asm/ldt.h>

struct hard_thread {
  bool created __attribute__((aligned (ARCH_CL_SIZE)));
  bool allocated;
  bool running __attribute__((aligned (ARCH_CL_SIZE)));
  void *tls_desc;
  struct user_desc ldt_entry;
  pthread_t thread;
};

/* Array of hard threads */
extern struct hard_thread *__ht_threads;
 
/* Now #include the externally exposed ht interface header so that internal
 * files only need to #include this header */
#include <ht.h>

#endif // HT_INTERNAL_H
