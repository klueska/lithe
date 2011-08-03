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

#define init_uthread_entry_ARCH(uth, entry) \
  makecontext(&(uth)->uc, (entry), 0)

#else // !__linux__
  #error "Your architecture is not yet supported by parlib!"
#endif 

#endif // PARLIB_ARCH_H
