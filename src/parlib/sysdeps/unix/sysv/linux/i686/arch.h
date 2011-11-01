#ifndef PARLIB_ARCH_H
#define PARLIB_ARCH_H 

#ifdef __linux__

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <ucontext.h>
typedef struct ucontext uthread_context_t;

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

#define init_uthread_stack_ARCH(uth, stack_top, size)   \
{                                                       \
	ucontext_t *uc = &(uth)->uc;                        \
	int ret = getcontext(uc);                           \
	assert(ret == 0);                                   \
	uc->uc_stack.ss_sp = (void*)(stack_top);            \
	uc->uc_stack.ss_size = (uint32_t)(size);            \
}

#define init_uthread_entry_ARCH(uth, entry) \
  makecontext(&(uth)->uc, (entry), 0)

#else // !__linux__
  #error "Your architecture is not yet supported by parlib!"
#endif 

#endif // PARLIB_ARCH_H
