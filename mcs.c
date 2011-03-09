#include <stdio.h>
#include <stdlib.h>

#include "atomic.h"
#include "mcs.h"

int ht_id();

void mcs_lock(mcs_lock_t *L, struct qnode *I)
{
/*   struct timeval start, end; */
/*   gettimeofday(&start, NULL); */

  I->next = NULL;
  struct qnode *predecessor = fetch_and_store(L, I);
  if (predecessor != NULL) {
    I->locked = 1;
    predecessor->next = I;
    while (coherent_read(I->locked) == 1);
/*     printf("(%d) slow mcs_lock(0x%x, 0x%x)\n", ht_id(), L, I); */
  } else {
/*     printf("(%d) fast mcs_lock(0x%x, 0x%x)\n", ht_id(), L, I); */
  }

/*   gettimeofday(&end, NULL); */
/*   long elapsed = ((end.tv_sec * 1000000 + end.tv_usec) */
/* 		  - (start.tv_sec * 1000000 + start.tv_usec)); */
/*   if (elapsed > 10) */
/*     printf("mcs_lock: %ld\n", elapsed); */

/*   if (elapsed > 100) */
/*     asm volatile ("int3"); */
}

void mcs_unlock(mcs_lock_t *L, struct qnode *I)
{
/*   struct timeval start, end; */
/*   gettimeofday(&start, NULL); */

  if (I->next == NULL) {
    if (__sync_bool_compare_and_swap(L, I, NULL)) {
/*       printf("(%d) fast mcs_unlock(0x%x, 0x%x)\n", ht_id(), L, I); */
/*       gettimeofday(&end, NULL); */
/*       long elapsed = ((end.tv_sec * 1000000 + end.tv_usec) */
/* 		      - (start.tv_sec * 1000000 + start.tv_usec)); */
/*       if (elapsed > 10) */
/* 	printf("mcs_unlock: %ld\n", elapsed); */
      return;
    }
    while (coherent_read(I->next) == (unsigned int)NULL);
  }
  I->next->locked = 0;
/*   printf("(%d) slow mcs_unlock(0x%x, 0x%x)\n", ht_id(), L, I); */
/*   gettimeofday(&end, NULL); */
/*   long elapsed = ((end.tv_sec * 1000000 + end.tv_usec) */
/* 		  - (start.tv_sec * 1000000 + start.tv_usec)); */
/*   if (elapsed > 10) */
/*     printf("mcs_unlock: %ld\n", elapsed); */
}
