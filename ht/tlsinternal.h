#ifndef HT_TLS_INTERNAL_H
#define HT_TLS_INTERNAL_H

#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <ht/arch.h>
#include <ht/glibc-tls.h>
#include <ht/htinternal.h>

/* This is the default entry number of the TLS segment in the GDT when a
 * process first starts up. */
#define DEFAULT_TLS_GDT_ENTRY 6

/* Linux fills in 5 LDT entries, but effectively only uses 2.  We'll assume 5
 * are reserved, just to be safe */
#define RESERVED_LDT_ENTRIES 5

/* Initialize tls for use by a ht */
void init_tls(uint32_t htid);

#define TLS_SET_GS(entry, ldt) \
  asm volatile("movl %0, %%gs" :: "r" ((entry<<3)|(0x4&(ldt<<2))|0x3) : "memory")

#define TLS_GET_GS() \
({ \
  int reg = 0; \
  asm volatile("movl %%gs, %0" : "=r" (reg)); \
  reg; \
})

#define TLS_REGION_ADDR() \
({ \
  void *reg = NULL; \
  asm volatile("movl %%gs:0x0, %0" : "=r" (reg)); \
  reg; \
})

#define TLS_PRINT_REGION_ADDR() \
({ \
  void *addr = TLS_REGION_ADDR(); \
  printf("addr: %p\n", addr); \
})

#define TLS_PRINT_GS() \
({ \
  int gs = TLS_GET_GS(); \
  printf("gs: entry:%d, ldt?:%d, perms:%d\n", (gs>>3), (0x1&(gs>>2)), (gs&0x3)); \
})

#include <ht/tls.h>

#endif /* HT_TLS_INTERNAL_H */

