/*
 * Copyright (c) 2011 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#include <stdbool.h>
#include <errno.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <parlib/mcs.h>
#include <parlib/vcore.h>
#include <parlib/uthread.h>
#include <parlib/parlib.h>
#include <ht/ht.h>

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
  vcore_entry();
}

/* Default callback for vcore_entry() */
static void __vcore_entry()
{
	// Do nothing by default...
}
extern void vcore_entry() __attribute__ ((weak, alias ("__vcore_entry")));

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

