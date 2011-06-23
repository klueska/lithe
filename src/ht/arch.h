/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef HT_ARCH_H
#define HT_ARCH_H

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#ifdef __i386__
  #define internal_function __attribute ((regparm (3), stdcall))
#elif __x86_64__
  #define internal_function
#endif

#define PGSIZE getpagesize()
#define ARCH_CL_SIZE 64

/* Make sure you subtract off/save enough space at the top of the stack for
 * whatever you compiler might want to use when calling a noreturn function or
 * to handle a HW spill or whatever. */
static __inline void __attribute__((always_inline))
set_stack_pointer(void* sp)
{
#ifdef __i386__ 
	asm volatile ("mov %0,%%esp" : : "r"(sp) : "memory","esp");
#elif __x86_64__ 
	asm volatile ("mov %0,%%rsp" : : "r"(sp) : "memory","rsp");
#endif
}

static __inline void
breakpoint(void)
{
	__asm __volatile("int3");
}

static __inline uint64_t
read_tsc(void)
{
	uint64_t tsc;
	__asm __volatile("rdtsc" : "=A" (tsc));
	return tsc;
}

static __inline void
cpu_relax(void)
{
	asm volatile("pause" : : : "memory");
}

static __inline uint64_t                                                                             
read_pmc(uint32_t index)
{                                                                                                    
    uint64_t pmc;

    __asm __volatile("rdpmc" : "=A" (pmc) : "c" (index)); 
    return pmc;                                                                                      
}

#endif /* HT_ARCH_H */
