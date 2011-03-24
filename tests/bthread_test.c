#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <ht/ht.h>
#include <bthread.h>
#include <parlib.h>

bthread_mutex_t lock = BTHREAD_MUTEX_INITIALIZER;
#define printf_safe(...) {}
//#define printf_safe(...) \
	bthread_mutex_lock(&lock); \
	printf(__VA_ARGS__); \
	bthread_mutex_unlock(&lock);

#define NUM_TEST_THREADS 10000
#define NUM_YIELDS 100

bthread_t my_threads[NUM_TEST_THREADS];
void *my_retvals[NUM_TEST_THREADS];

__thread int my_id;
void *yield_thread(void* arg)
{	
	assert(!in_vcore_context());
	for (int i = 0; i < NUM_YIELDS; i++) {
		printf_safe("[A] bthread %d on vcore %d\n", bthread_self()->id, vcore_id());
		bthread_yield();
		printf_safe("[A] bthread %d returned from yield on vcore %d\n",
		            bthread_self()->id, vcore_id());
	}
	return (void*)(bthread_self()->id);
}

int main(int argc, char** argv) 
{
	/* Create and join on threads */
	int itr = 0;
	while (1) {
		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			printf_safe("[A] About to create thread %d\n", i);
			bthread_create(&my_threads[i], NULL, &yield_thread, NULL);
		}
		for (int i = 0; i < NUM_TEST_THREADS; i++) {
			printf_safe("[A] About to join on thread %d\n", i);
			bthread_join(my_threads[i], &my_retvals[i]);
			printf_safe("[A] Successfully joined on thread %d (retval: %p)\n", i,
			            my_retvals[i]);
		}
		printf("Done: i=%d\n", itr++);
//		fflush(stdout);
		break;
	}
} 
