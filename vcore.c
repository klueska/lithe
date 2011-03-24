#include <stdbool.h>
#include <errno.h>
#include <vcore.h>
#include <mcs.h>
#include <sys/param.h>
#include <parlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <uthread.h>
#include <ht/ht.h>

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
	ht_yield();
}

/* Clear pending, and try to handle events that came in between a previous call
 * to handle_events() and the clearing of pending.  While it's not a big deal,
 * we'll loop in case we catch any.  Will break out of this once there are no
 * events, and we will have send pending to 0. 
 *
 * Note that this won't catch every race/case of an incoming event.  Future
 * events will get caught in pop_ros_tf() */
void clear_notif_pending(uint32_t vcoreid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

/* Enables notifs, and deals with missed notifs by self notifying.  This should
 * be rare, so the syscall overhead isn't a big deal. */
void enable_notifs(uint32_t vcoreid)
{
//	printf("Figure out how to properly implement %s on linux!\n", __FUNCTION__);
}

