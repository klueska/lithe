#include <tlsinternal.h>


static struct user_desc main_ud;

/* Get a TLS, returns 0 on failure.  Any thread created by a user-level
 * scheduler needs to create a TLS. */
void *allocate_tls(void)
{
	extern void *_dl_allocate_tls(void *mem) internal_function;
	void *tcb = _dl_allocate_tls(NULL);
	if (!tcb)
		return 0;
	/* Make sure the TLS is set up properly - its tcb pointer points to itself.
	 * Keep this in sync with sysdeps/ros/XXX/tls.h.  For whatever reason,
	 * dynamically linked programs do not need this to be redone, but statics
	 * do. */
	memcpy(tcb, (void*)main_ud.base_addr, sizeof(tcbhead_t));
	tcbhead_t *head = (tcbhead_t*)tcb;
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
	main_ud.entry_number = DEFAULT_TLS_GDT_ENTRY;
	int ret = syscall(SYS_get_thread_area, &main_ud);
	assert(ret == 0);
}

/* Initialize tls for use in this ht */
void init_tls(uint32_t htid)
{
	/* Allocate a new tls region for this ht */
	void *tcb = allocate_tls();
	assert(tcb);

	/* Set up the the new TLS region as an entry in the LDT */
	struct user_desc *ud = &(__ht_threads[htid].ldt_entry);
	memset(ud, 0, sizeof(struct user_desc));
	ud->entry_number = htid;
	ud->limit = 0xffffffff;
	ud->seg_32bit = 1;
	ud->limit_in_pages = 1;
	ud->useable = 1;

	/* Switch to this new TLS region */
	__ht_threads[htid].tls_desc = tcb;
	set_tls_desc(tcb, htid);
}

/* Passing in the htid, since it'll be in TLS of the caller */
void set_tls_desc(void *tls_desc, uint32_t htid)
{
  assert(tls_desc != NULL);

  struct user_desc *ud = &(__ht_threads[htid].ldt_entry);
  ud->base_addr = (unsigned long int)tls_desc;
  int ret = syscall(SYS_modify_ldt, 1, ud, sizeof(struct user_desc));
  assert(ret == 0);
  TLS_SET_GS(htid, 1);
}

/* Get the tls descriptor currently set for a given hard thread. This should
 * only ever be called once the hard thread has been initialized */
void *get_tls_desc(uint32_t htid)
{
	struct user_desc *ud = &(__ht_threads[htid].ldt_entry);
	assert(ud->base_addr != 0);
	return (void *)ud->base_addr;
}

