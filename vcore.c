/*
 * Copyright (c) 2011 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#include <stdbool.h>
#include <errno.h>
#include <vcore.h>
#include <sys/param.h>
#include <parlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <uthread.h>
#include <ht/ht.h>
#include <ht/mcs.h>

static mcs_lock_t __ht_mutex = MCS_LOCK_INIT;

void vcore_entry()
{
	/* Just call up into the uthread library */
	uthread_vcore_entry();
}

/* Entry point from an underlying hard thread */
void ht_entry()
{
	if(current_ucontext) {
		memcpy(&current_uthread->uc, current_ucontext, sizeof(struct ucontext));
		current_uthread->tls_desc = current_tls_desc;
		current_tls_desc = NULL;
		current_ucontext = NULL;
	}
	vcore_entry();
}

/* Returns -1 with errno set on error, or 0 on success.  This does not return
 * the number of cores actually granted (though some parts of the kernel do
 * internally).
 *
 * Note the doesn't block or anything (despite the min number requested is
 * 1), since the kernel won't block the call. */
int vcore_request(size_t k)
{
	return ht_request_async(k);
}

void vcore_yield()
{
	struct mcs_lock_qnode local_qn = {0};
	mcs_lock_lock(&__ht_mutex, &local_qn);
	ht_yield(&__ht_mutex, &local_qn);
}

