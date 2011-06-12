#ifndef LITHE_ARCH_H
#define LITHE_ARCH_H 

#include <ht/arch.h>

/**
 * Grant current hard thread to specified scheduler. The specified
 * scheduler must be non-null and a registered scheduler of the
 * current scheduler. This function never returns unless child is
 * NULL, in which case it sets errno appropriately and returns -1.
 */
#ifdef __x86_64__
#ifdef __PIC__
#define lithe_sched_enter_ARCH(child)                                      \
({                                                                    \
  asm volatile ("mov %0, %%rdi;"                                      \
		"jmp ___lithe_sched_enter@PLT"                        \
		: : "r" (child));                                     \
})
#else
#define lithe_sched_enter_ARCH(child)                                      \
({                                                                    \
  asm volatile ("mov %0, %%rdi;"                                      \
                "jmp ___lithe_sched_enter"                            \
		: : "r" (child));                                     \
})
#endif /* __PIC__ */
#elif __i386__
# ifdef __PIC__
# define lithe_sched_enter_ARCH(child)                                      \
({                                                                    \
  asm volatile ("mov %0, %%edi;"                                      \
		"jmp ___lithe_sched_enter@PLT"                        \
		: : "r" (child));                                     \
})
# else
# define lithe_sched_enter_ARCH(child)                                      \
({                                                                    \
  __lithe_sched_enter(child);                            \
})
# endif /* __PIC__ */
#else
# error "missing implementations for your architecture"
#endif

/**
 * Yield current hard thread to parent scheduler. This function should
 * never return.
 */
#ifdef __PIC__
#define lithe_sched_yield_ARCH()                                      \
({                                                                    \
  asm volatile ("jmp ___lithe_sched_yield@PLT");                      \
})
#else
#define lithe_sched_yield_ARCH()                                      \
({                                                                    \
  asm volatile ("jmp ___lithe_sched_yield");                          \
})
#endif /* __PIC__ */

/**
 * Reenter current scheduler. This function should never return.
 */
#ifdef __PIC__
#define lithe_sched_reenter_ARCH()                                         \
({                                                                    \
  asm volatile ("jmp ___lithe_sched_reenter@PLT");                    \
})
#else
#define lithe_sched_reenter_ARCH()                                         \
({                                                                    \
  asm volatile ("jmp ___lithe_sched_reenter");                        \
})
#endif /* __PIC__ */

#endif
