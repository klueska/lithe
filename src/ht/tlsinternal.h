/*
 * Copyright (c) 2011 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef HT_TLS_INTERNAL_H
#define HT_TLS_INTERNAL_H

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <ht/arch.h>
#include <ht/glibc-tls.h>
#include <ht/htinternal.h>

#ifdef __x86_64__
  #include <asm/prctl.h>
  #include <sys/prctl.h>
  int arch_prctl(int code, unsigned long *addr);
#endif 

/* This is the default entry number of the TLS segment in the GDT when a
 * process first starts up. */
#define DEFAULT_TLS_GDT_ENTRY 6

/* Linux fills in 5 LDT entries, but effectively only uses 2.  We'll assume 5
 * are reserved, just to be safe */
#define RESERVED_LDT_ENTRIES 5

/* Initialize the tls subsystem for use */
int tls_lib_init();

/* Initialize tls for use by a ht */
void init_tls(uint32_t htid);

/* Get the current tls base address */
static __inline void *get_current_tls_base()
{
	size_t addr;
#ifdef __i386__
    struct user_desc ud;
	ud.entry_number = DEFAULT_TLS_GDT_ENTRY;
	int ret = syscall(SYS_get_thread_area, &ud);
	assert(ret == 0);
	assert(ud.base_addr);
    addr = (size_t)ud.base_addr;
#elif __x86_64__
	arch_prctl(ARCH_GET_FS, &addr);
#endif
	return (void *)addr;
}

#ifdef __i386__
  #define TLS_SET_SEGMENT_REGISTER(entry, ldt) \
    asm volatile("movl %0, %%gs" :: "r" ((entry<<3)|(0x4&(ldt<<2))|0x3) : "memory")

  #define TLS_GET_SEGMENT_REGISTER(entry, ldt, perms) \
  ({ \
    int reg = 0; \
    asm volatile("movl %%gs, %0" : "=r" (reg)); \
    printf("reg: 0x%x\n", reg); \
    *(entry) = (reg >> 3); \
    *(ldt) = (0x1 & (reg >> 2)); \
    *(perms) = (0x3 & reg); \
  })

#elif __x86_64
  #define TLS_SET_SEGMENT_REGISTER(entry, ldt) \
    asm volatile("movl %0, %%fs" :: "r" ((entry<<3)|(0x4&(ldt<<2))|0x3) : "memory")

  #define TLS_GET_SEGMENT_REGISTER(entry, ldt, perms) \
  ({ \
    int reg = 0; \
    asm volatile("movl %%fs, %0" : "=r" (reg)); \
    printf("reg: 0x%x\n", reg); \
    *(entry) = (reg >> 3); \
    *(ldt) = (0x1 & (reg >> 2)); \
    *(perms) = (0x3 & reg); \
  })

#endif

#include <ht/tls.h>

#endif /* HT_TLS_INTERNAL_H */

