#include <stddef.h>
#include <sys/syscall.h>
#include <ht/atomic.h>
#include <ht/htinternal.h>
#include <ht/tlsinternal.h>

void *__ht_main_tls_desc;

/* Get a TLS, returns 0 on failure.  Any thread created by a user-level
 * scheduler needs to create a TLS. */
void *allocate_tls(void)
{
	extern void *_dl_allocate_tls(void *mem) internal_function;
	void *tcb = _dl_allocate_tls(NULL);
	if (!tcb)
		return 0;
	/* Make sure the TLS is set up properly - its tcb pointer 
	 * points to itself. */
	tcbhead_t *head = (tcbhead_t*)tcb;
	size_t offset = offsetof(tcbhead_t, multiple_threads);

	/* These fields in the tls_desc need to be set up for linux to work
	 * properly with TLS. Take a look at how things are done in libc in the
	 * nptl code for reference. */
	memcpy(tcb+offset, __ht_main_tls_desc+offset, sizeof(tcbhead_t)-offset);
	head->tcb = tcb;
	head->self = tcb;
	return tcb;
}

/* Free a previously allocated TLS region */
void free_tls(void *tcb)
{
	extern void _dl_deallocate_tls (void *tcb, bool dealloc_tcb) internal_function;
	assert(tcb);
	_dl_deallocate_tls(tcb, 1);
}

/* Constructor to get a reference to the main thread's TLS descriptor */
static void __attribute((constructor)) __tls_ctor()
{
	/* Get a reference to the main program's TLS descriptor */
    struct user_desc ud;
	ud.entry_number = DEFAULT_TLS_GDT_ENTRY;
	int ret = syscall(SYS_get_thread_area, &ud);
	assert(ret == 0);
	assert(ud.base_addr);
    __ht_main_tls_desc = (tcbhead_t*)ud.base_addr;
}

/* Initialize tls for use in this ht */
void init_tls(uint32_t htid)
{
	/* Get a reference to the current TLS region in the GDT */
    struct user_desc cud;
	cud.entry_number = DEFAULT_TLS_GDT_ENTRY;
	int ret = syscall(SYS_get_thread_area, &cud);
	assert(ret == 0);
	void *tcb = (void*)cud.base_addr;
	assert(tcb);

	/* Set up the TLS region as an entry in the LDT */
	struct user_desc *ud = &(__ht_threads[htid].ldt_entry);
	memset(ud, 0, sizeof(struct user_desc));
	ud->entry_number = htid + RESERVED_LDT_ENTRIES;
	ud->limit = 0xffffffff;
	ud->seg_32bit = 1;
	ud->limit_in_pages = 1;
	ud->useable = 1;

	/* Set the tls_desc in the tls_desc array */
	ht_tls_descs[htid] = tcb;
}

/* Passing in the htid, since it'll be in TLS of the caller */
void set_tls_desc(void *tls_desc, uint32_t htid)
{
  assert(tls_desc != NULL);

  struct user_desc *ud = &(__ht_threads[htid].ldt_entry);
  ud->base_addr = (unsigned long int)tls_desc;
  int ret = syscall(SYS_modify_ldt, 1, ud, sizeof(struct user_desc));
  assert(ret == 0);
  TLS_SET_GS(ud->entry_number, 1);
  __ht_id = htid;
}

/* Get the tls descriptor currently set for a given hard thread. This should
 * only ever be called once the hard thread has been initialized */
void *get_tls_desc(uint32_t htid)
{
	struct user_desc *ud = &(__ht_threads[htid].ldt_entry);
	assert(ud->base_addr != 0);
	return (void *)ud->base_addr;
}

