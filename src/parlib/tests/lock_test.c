#include <parlib/parlib.h>
#include <parlib/vcore.h>
#include <parlib/mcs.h>
#include <assert.h>
#include <stdio.h>

static mcs_lock_t lock = MCS_LOCK_INIT;

int main(int argc, char** argv)
{
	/* Make sure we are not in vcore context */
	assert(!in_vcore_context());

	/* Request as many as the system will give us */
	vcore_request(max_vcores());
	vcore_yield(false);
	assert(0);
}

int global_count = 0;

void vcore_entry(void)
{
	/* Make sure we are in vcore context */
	assert(in_vcore_context());

	if(ht_saved_ucontext) {
      setcontext(ht_saved_ucontext);
	}

	/* Test the locks forever.  Should never dead lock */
	printf("Begin infinite loop on vcore %d\n", vcore_id());
	uint32_t vcoreid = vcore_id();
	mcs_lock_qnode_t qnode = {0};
	while(1) {
		memset(&qnode, 0, sizeof(mcs_lock_qnode_t));
		mcs_lock_lock(&lock, &qnode);
		printf("global_count: %d, on vcore %d\n", global_count++, vcoreid);
		mcs_lock_unlock(&lock, &qnode);
	}
	assert(0);
}

