#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <ht/ht.h>
#include <ht/atomic.h>
#include <parlib.h>
#include <bthread.h>

bthread_mutex_t lock = BTHREAD_MUTEX_INITIALIZER;
#define printf_safe(...) {}
//#define printf_safe(...) printf(__VA_ARGS__)
//#define printf_safe(...) \
//       bthread_mutex_lock(&lock); \
//       printf(__VA_ARGS__); \
//       bthread_mutex_unlock(&lock);
//#define write_safe(max_len, ...) \
//{ \
//	char buf[max_len]; \
//	sprintf(buf, __VA_ARGS__); \
//	int amt = write(1, buf, max_len); \
//	assert(amt); \
//	bthread_mutex_unlock(&lock); \
//}

#define NUM_TEST_THREADS 150000
#define NUM_YIELDS 10000

bthread_t my_threads[NUM_TEST_THREADS];
void *my_retvals[NUM_TEST_THREADS];
int threads_yielded = 0;
int yield_itrs = 0;

void *yield_thread(void* arg)
{	
	for (int i = 0; i < NUM_YIELDS; i++) {
		assert(!in_vcore_context());
		bthread_yield();
        bthread_mutex_lock(&lock);
		if(++threads_yielded == NUM_TEST_THREADS) {
			//write_safe(18, "yield_itrs: %d\n", ++yield_itrs);
			printf_safe("yield_itrs: %d\n", ++yield_itrs);
			threads_yielded = 0;
		}
        bthread_mutex_unlock(&lock);
		//printf_safe("[A] bthread %d returned from yield on vcore %d\n",
		//            bthread_self()->id, vcore_id());
	}
	return (void*)(bthread_self()->id);
}

int main(int argc, char** argv) 
{
	/* Create and join on threads */
	printf("Begin test...\n");
	while (1) {
		printf("Creating threads...\n");
		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			//printf_safe("[A] About to create thread %d, vcoreid: %d\n", i, vcore_id());
			bthread_create(&my_threads[i], NULL, &yield_thread, NULL);
		}
		printf("Joining threads...\n");
		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			//printf_safe("[A] About to join on thread %d\n", i);
			bthread_join(my_threads[i], &my_retvals[i]);
			//printf_safe("[A] Successfully joined on thread %d (retval: %p)\n", i,
			//            my_retvals[i]);
		}
		break;
	}
} 
