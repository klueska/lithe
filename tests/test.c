#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <ht.h>

void ht_entry()
{
  printf("entry %d, num_hts: %d\n", ht_id(), ht_num_hard_threads());
  if(current_ht_context) {
    setcontext(current_ht_context);
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
