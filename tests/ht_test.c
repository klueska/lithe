#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <ht/ht.h>
#include <ht/tls.h>

void ht_entry()
{
  printf("entry %d, num_hts: %d\n", ht_id(), ht_num_hard_threads());
  if(current_ucontext) {
    set_tls_desc(current_tls_desc, ht_id());
    setcontext(current_ucontext);
    assert(0);
  }

  ht_request_async(ht_limit_hard_threads());
  ht_yield();
}

int main()
{
  printf("main, limit_hts: %d\n", ht_limit_hard_threads());
  ht_request_async(ht_limit_hard_threads());
  sleep(10);
  return 0;
}
