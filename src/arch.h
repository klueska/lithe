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

#define init_uthread_entry_ARCH(uth, entry, argc, ...)                  \
{                                                                       \
    if(argc == 0)                                                       \
	  init_uthread_entry_0_x86_64(uth, entry, ##__VA_ARGS__);           \
    else if(argc == 1)                                                  \
	  init_uthread_entry_1_x86_64(uth, entry, ##__VA_ARGS__);           \
    else if(argc == 2)                                                  \
	  init_uthread_entry_2_x86_64(uth, entry, ##__VA_ARGS__);           \
    else if(argc == 3)                                                  \
	  init_uthread_entry_3_x86_64(uth, entry, ##__VA_ARGS__);           \
}

#define init_uthread_entry_0_x86_64(uth, entry, ...) \
  makecontext(&(uth)->uc, (void*)entry, 0, ##__VA_ARGS__)

#define init_uthread_entry_1_x86_64(uth, entry, arg)     \
{                                                               \
  long unsigned int targ1 = arg1;                               \
  makecontext(&(uth)->uc, makecontext_entry_1_x86_64, 4,        \
              entry >> 32, entry & 0xFFFFFFFF,                  \
              (targ) >> 32, (targ) & 0xFFFFFFFF)                \
}

#define init_uthread_entry_2_x86_64(uth, entry, arg1, arg2)     \
{                                                               \
  long unsigned int targ1 = arg1;                               \
  long unsigned int targ2 = arg2;                               \
  makecontext(&(uth)->uc, makecontext_entry_2_x86_64, 6,        \
              entry >> 32, entry & 0xFFFFFFFF,                  \
              (targ1) >> 32, (targ1) & 0xFFFFFFFF,              \
              (targ2) >> 32, (targ2) & 0xFFFFFFFF)              \
}

#define init_uthread_entry_3_x86_64(uth, entry, arg1, arg2, arg3)     \
{                                                                     \
  long unsigned int targ1 = arg1;                                     \
  long unsigned int targ2 = arg2;                                     \
  long unsigned int targ3 = arg3;                                     \
  makecontext(&(uth)->uc, makecontext_entry_3_x86_64, 8,              \
              entry >> 32, entry & 0xFFFFFFFF,                        \
              (targ1) >> 32, (targ1) & 0xFFFFFFFF,                    \
              (targ2) >> 32, (targ2) & 0xFFFFFFFF,                    \
              (targ3) >> 32, (targ3) & 0xFFFFFFFF)                    \
}

static void makecontext_entry_1_x86_64(int entry1, int entry2, 
                                       int arg1, int arg2)
{
  void (*entry) (void *) = (void (*) (void *))
    (((unsigned long) entry1 << 32) + (unsigned int) entry2);
  void *arg = (void *)
    (((unsigned long) arg1 << 32) + (unsigned int) arg2);
  entry(arg);
}

static void makecontext_entry_2_x86_64(int entry1, int entry2, 
                                       int arg1, int arg2,
                                       int arg3, int arg4)
{
  void (*entry) (void *) = (void (*) (void *))
    (((unsigned long) entry1 << 32) + (unsigned int) entry2);
  void *new_arg1 = (void *)
    (((unsigned long) arg1 << 32) + (unsigned int) arg2);
  void *new_arg2 = (void *)
    (((unsigned long) arg3 << 32) + (unsigned int) arg4);
  entry(new_arg1, new_arg2);
}

static void makecontext_entry_3_x86_64(int entry1, int entry2, 
                                       int arg1, int arg2,
                                       int arg3, int arg4,
                                       int arg5, int arg6)
{
  void (*entry) (void *) = (void (*) (void *))
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
