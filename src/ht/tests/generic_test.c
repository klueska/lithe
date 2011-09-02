#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <asm/ldt.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/param.h>
#include <glibc-tls.h>

#ifdef __i386__
  #define internal_function   __attribute ((regparm (3), stdcall))
#elif __x86_64
  #define internal_function
#endif

extern void *_dl_allocate_tls(void *mem) internal_function; 
extern void _dl_deallocate_tls (void *tcb, bool dealloc_tcb) internal_function; 

__thread int huge_tls_array[1000];
void *main_tls_area;

void do_work(char *m, int msize, char *mm, int mmsize, char *tcb, int tcbsize)
{
	for(int i=0; i<1000; i++)
		huge_tls_array[i] = i;
	for(int i=0; i<msize; i++)
		m[i] = i;
	for(int i=0; i<mmsize; i++)
		mm[i] = i;

//	memcpy(tcb, (tcbhead_t *)main_tls_area, sizeof(tcbhead_t));
	tcbhead_t *head = (tcbhead_t*)tcb;
	head->tcb = tcb;
	head->self = tcb;

	struct user_desc ud;
	memset(&ud, 0, sizeof(struct user_desc));
	ud.base_addr = 0;
	ud.entry_number = 10;
	ud.limit = 0xffffffff;
	ud.seg_32bit = 1;
	ud.limit_in_pages = 1;
	ud.useable = 1;

	int ret = syscall(SYS_modify_ldt, 1, &ud, sizeof(struct user_desc));
	assert(ret == 0);
}

void *entry(void *arg)
{
	#define MMAP_SIZE (4*4096)
	//#define MALLOC_SIZE (MIN((rand()/4096)+1, MMAP_SIZE))
	#define MALLOC_SIZE MMAP_SIZE
	char *m;
	char *mm;
	char *tcb;

	while(1) {
		int msize = MALLOC_SIZE;
		int mmsize = MMAP_SIZE;
		int tcbsize = 0;
		m = calloc(1, msize);
		mm = mmap(0, MMAP_SIZE,
		          PROT_READ|PROT_WRITE|PROT_EXEC,
		          MAP_SHARED|MAP_POPULATE|MAP_ANONYMOUS, -1, 0);
		tcb = _dl_allocate_tls(NULL);
		assert(m);
		assert(mm);
		assert(tcb);

		do_work(m, msize, mm, mmsize, tcb, tcbsize);

		_dl_deallocate_tls(tcb, 1);
		assert(!munmap(mm, mmsize));
		free(m);
//		printf("i: %d\n", (int)arg);
//		fflush(stdout);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	struct user_desc ud;
	ud.entry_number = 6;
	int ret = syscall(SYS_get_thread_area, &ud);
	assert(ret == 0);
	main_tls_area = (void *)(long)ud.base_addr;

	#define NUM_PTHREADS 10
	pthread_t pthreads[NUM_PTHREADS];
	for(int i=0; i<NUM_PTHREADS; i++)
		pthread_create(&pthreads[i], NULL, entry, (void *)(long)i);
	for(int i=0; i<NUM_PTHREADS; i++)
		pthread_join(pthreads[i], NULL);
	return 0;
}
