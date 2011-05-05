#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <ht/ht.h>
#include <ht/tls.h>
#include <ht/atomic.h>
#include <ht/mcs.h>

#define NUM_HTS \
  ht_limit_hard_threads()

void ht_entry()
{
  if(current_ucontext) {
    void *cuc = current_ucontext;
    printf("Restoring context: entry %d, num_hts: %d\n", ht_id(), ht_num_hard_threads());
    set_tls_desc(current_tls_desc, ht_id());
    setcontext(cuc);
    assert(0);
  }

  printf("entry %d, num_hts: %d\n", ht_id(), ht_num_hard_threads());
  mcs_lock_lock(&ht_yield_lock, &ht_yield_qnode);
  ht_request_async(NUM_HTS);
  ht_yield();
}

int main()
{
  printf("main, limit_hts: %d\n", ht_limit_hard_threads());
  ht_request_async(NUM_HTS);
  set_tls_desc(ht_tls_descs[0], 0);
  current_ucontext = NULL;  
  ht_entry();
}

