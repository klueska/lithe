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
#include <mcs.h>
#include <ht/ht.h>
#include <uthread.h>

/**
 * User defined callback function signalling that the ht libary is done
 * initializing itself. We simply reflect this back up as a vcore_ready()
 * callback.
 */
void ht_ready()
{
  vcore_ready();
}

/* Default callback for vcore_ready() */
static void __vcore_ready()
{
	// Do nothing by default...
}
extern void vcore_ready() __attribute__ ((weak, alias ("__vcore_ready")));

/* Entry point from an underlying hard thread */
void ht_entry()
{
	if(ht_saved_ucontext) {
		memcpy(&current_uthread->uc, ht_saved_ucontext, sizeof(struct ucontext));
		current_uthread->tls_desc = ht_saved_tls_desc;
	}
	uthread_vcore_entry();
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
	ht_lock(&ht_yield_lock);
	ht_yield();
}

