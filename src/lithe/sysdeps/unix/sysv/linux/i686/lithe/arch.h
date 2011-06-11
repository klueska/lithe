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

//#define has_trampoline() (trampoline.uth.uc.uc_stack.ss_sp != NULL)
//
//#ifdef __x86_64__
//# define swap_to_trampoline()						\
//{									\
//  task = &trampoline;							\
//  void *stack = (void *)						\
//    (((size_t) trampoline.uth.uc.uc_stack.ss_sp)				\
//     + (trampoline.uth.uc.uc_stack.ss_size - __alignof__(long double)));	\
//  asm volatile ("mov %0, %%rsp" : : "r" (stack));			\
//}
//#elif __i386__
//# define swap_to_trampoline()                              \
//{                                                          \
//  task = &trampoline;                                      \
//  void *stack = (void *)                                   \
//    (trampoline.uth.uc.uc_stack.ss_sp                      \
//     + trampoline.uth.uc.uc_stack.ss_size);                \
//  set_stack_pointer(stack);                                \
//}
//#else
//# error "missing implementations for your architecture"
//#endif
//
///* TODO(benh): Implement this assuming no stack! */
//#define setup_trampoline()                                                   \
//{                                                                            \
//  /* Determine trampoline stack size. */                                     \
//  int pagesize = getpagesize();                                              \
//                                                                             \
//  size_t size = pagesize * 4;                                                \
//                                                                             \
//  const int protection = (PROT_READ | PROT_WRITE | PROT_EXEC);               \
//  const int flags = (MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT);               \
//                                                                             \
//  void *stack = mmap(NULL, size, protection, flags, -1, 0);                  \
//                                                                             \
//  if (stack == MAP_FAILED) {                                                 \
//    fatalerror("lithe: could not allocate trampoline");                      \
//  }                                                                          \
//                                                                             \
//  /* Disallow all memory access to the last page. */                         \
//  if (mprotect(stack, pagesize, PROT_NONE) != 0) {                           \
//    fatalerror("lithe: could not protect page");                             \
//  }                                                                          \
//                                                                             \
//  /* Prepare context. */                                                     \
//  if (getcontext(&trampoline.uth.uc)) {                                      \
//    fatalerror("lithe: could not get context");                              \
//  }                                                                          \
//                                                                             \
//  trampoline.uth.uc.uc_stack.ss_sp = stack;                                  \
//  trampoline.uth.uc.uc_stack.ss_size = size;                                 \
//  trampoline.uth.uc.uc_link = 0;                                             \
//}
//
//#define cleanup_trampoline()                                                 \
//{                                                                            \
//  /* TODO(benh): This is Linux specific, should be in sysdeps/ */            \
//  void *stack = trampoline.uth.uc.uc_stack.ss_sp;                            \
//  size_t size = trampoline.uth.uc.uc_stack.ss_size;                          \
//                                                                             \
//  long result;                                                               \
//  asm volatile ("syscall"                                                    \
//               : "=a" (result)                                               \
//               : "0" (__NR_munmap),                                          \
//                  "D" ((long) stack),                                        \
//                 "S" ((long) size)                                           \
//               : "r11", "rcx", "memory");                                    \
//                                                                             \
//  /* TODO(benh): Compiler might use the stack for conditional! */            \
//  if ((unsigned long) (result) >= (unsigned long) (-127)) {                  \
//    errno = -(result);                                                       \
//    fatalerror("lithe: could not free trampoline");                          \
//  }                                                                          \
//                                                                             \
//  trampoline.uth.uc.uc_stack.ss_sp = NULL;                                   \
//  trampoline.uth.uc.uc_stack.ss_size = 0;                                    \
//}

#endif
