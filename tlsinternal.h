#ifndef HT_TLS_INTERNAL_H
#define HT_TLS_INTERNAL_H

#define DEFAULT_TLS_GDT_ENTRY 6

#define TLS_SET_GS(entry, ldt) \
  __asm ("movl %0, %%gs" :: "r" ((entry<<3)|(0x4&(ldt<<2))|0x3) : "memory")

#include <arch.h>
#include <stdio.h>
#include <assert.h>
#include <glibc-tls.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <htinternal.h>
#include <tls.h>

#endif /* HT_TLS_INTERNAL_H */

