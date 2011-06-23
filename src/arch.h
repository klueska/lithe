#ifndef PARLIB_ARCH_H
#define PARLIB_ARCH_H 

#ifdef __linux__

#define vcore_set_tls_var_ARCH(vcoreid, name, val)                \
{                                                                 \
	int vid = (vcoreid);                                          \
	typeof(name) temp_val = (val);                                \
	void *temp_tls_desc = current_tls_desc;                       \
    set_tls_desc(ht_tls_descs[vid], vid);                         \
	name = temp_val;                                              \
	set_tls_desc(temp_tls_desc, vid);                             \
}

#define vcore_get_tls_var_ARCH(vcoreid, name)                     \
({                                                                \
	int vid = (vcoreid);                                          \
	typeof(name) val;                                             \
	void *temp_tls_desc = current_tls_desc;                       \
    set_tls_desc(ht_tls_descs[vid], vid);                         \
	val = name;                                                   \
	set_tls_desc(temp_tls_desc, vid);                             \
    val;                                                          \
})

#include <ucontext.h>
typedef struct ucontext uthread_context_t;

#define init_uthread_stack_ARCH(uth, stack_top, size)   \
{                                                       \
	ucontext_t *uc = &(uth)->uc;                        \
    int ret = getcontext(uc);                           \
	assert(ret == 0);                                   \
	uc->uc_stack.ss_sp = (void*)(stack_top);            \
	uc->uc_stack.ss_size = (uint32_t)(size);            \
}

#ifdef __i386__

#define init_uthread_entry_ARCH(uth, entry, ...) \
  makecontext(&(uth)->uc, (void*)entry, ##__VA_ARGS__)

#elif __x86_64__

#define init_uthread_entry_ARCH(uth, entry, argc, ...) \
  init_uthread_entry_##argc##_ARCH(uth, entry, ##__VA_ARGS__) \

#define init_uthread_entry_0_ARCH(uth, entry) \
  makecontext(&(uth)->uc, (void*)entry, 0)

#define init_uthread_entry_1_ARCH(uth, entry, arg)                    \
{                                                                     \
  unsigned long tentry = (unsigned long) entry;                       \
  unsigned long targ = (unsigned long) arg;                           \
  makecontext(&(uth)->uc, (void *)makecontext_entry_1_ARCH, 4,        \
              tentry >> 32, tentry & 0xFFFFFFFF,                      \
              targ >> 32, targ & 0xFFFFFFFF);                         \
}

#define init_uthread_entry_2_ARCH(uth, entry, arg1, arg2)             \
{                                                                     \
  unsigned long tentry = (unsigned long) entry;                       \
  unsigned long targ1 = (unsigned long) arg1;                         \
  unsigned long targ2 = (unsigned long) arg2;                         \
  makecontext(&(uth)->uc, (void *)makecontext_entry_2_ARCH, 6,        \
              tentry >> 32, tentry & 0xFFFFFFFF,                      \
              targ1 >> 32, targ1 & 0xFFFFFFFF,                        \
              targ2 >> 32, targ2 & 0xFFFFFFFF);                       \
}

#define init_uthread_entry_3_ARCH(uth, entry, arg1, arg2, arg3)       \
{                                                                     \
  unsigned long tentry = (unsigned long) entry;                       \
  unsigned long targ1 = (unsigned long) arg1;                         \
  unsigned long targ2 = (unsigned long) arg2;                         \
  unsigned long targ3 = (unsigned long) arg3;                         \
  makecontext(&(uth)->uc, (void *)makecontext_entry_3_ARCH, 8,        \
              tentry >> 32, tentry & 0xFFFFFFFF,                      \
              targ1 >> 32, targ1 & 0xFFFFFFFF,                        \
              targ2 >> 32, targ2 & 0xFFFFFFFF,                        \
              targ3 >> 32, targ3 & 0xFFFFFFFF);                       \
}

static void makecontext_entry_1_ARCH(int entry1, int entry2, 
                                       int arg1, int arg2)
{
  void (*entry) (void *) = (void (*) (void *))
    (((unsigned long) entry1 << 32) + (unsigned int) entry2);
  void *arg = (void *)
    (((unsigned long) arg1 << 32) + (unsigned int) arg2);
  entry(arg);
}

static void makecontext_entry_2_ARCH(int entry1, int entry2, 
                                       int arg1, int arg2,
                                       int arg3, int arg4)
{
  void (*entry) (void *, void *) = (void (*) (void *, void *))
    (((unsigned long) entry1 << 32) + (unsigned int) entry2);
  void *new_arg1 = (void *)
    (((unsigned long) arg1 << 32) + (unsigned int) arg2);
  void *new_arg2 = (void *)
    (((unsigned long) arg3 << 32) + (unsigned int) arg4);
  entry(new_arg1, new_arg2);
}

static void makecontext_entry_3_ARCH(int entry1, int entry2, 
                                       int arg1, int arg2,
                                       int arg3, int arg4,
                                       int arg5, int arg6)
{
  void (*entry) (void *, void *, void *) = (void (*) (void *, void *, void *))
    (((unsigned long) entry1 << 32) + (unsigned int) entry2);
  void *new_arg1 = (void *)
    (((unsigned long) arg1 << 32) + (unsigned int) arg2);
  void *new_arg2 = (void *)
    (((unsigned long) arg3 << 32) + (unsigned int) arg4);
  void *new_arg3 = (void *)
    (((unsigned long) arg5 << 32) + (unsigned int) arg6);
  entry(new_arg1, new_arg2, new_arg3);
}

#endif // __x86_64__
#else // !__linux__
  #error "Your architecture is not yet supported by parlib!"
#endif 

#endif // PARLIB_ARCH_H
