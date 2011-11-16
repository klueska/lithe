#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <parlib/vcore.h>
#include <parlib/tls.h>
#include <parlib/atomic.h>
#include <pthread.h>

#define NUM_VCORES \
  limit_vcores()

void vcore_entry()
{
  if(vcore_saved_ucontext) {
    void *cuc = vcore_saved_ucontext;
    printf("Restoring context: entry %d, num_vcores: %d\n", vcore_id(), num_vcores());
    set_tls_desc(current_tls_desc, vcore_id());
    setcontext(cuc);
    assert(0);
  }

  printf("entry %d, num_vcores: %d\n", vcore_id(), num_vcores());
  vcore_request(1);
  vcore_yield();
}

int main()
{
  vcore_lib_init();
  printf("main, limit_vcores: %d\n", limit_vcores());
  vcore_request(NUM_VCORES);
  set_tls_desc(vcore_tls_descs[0], 0);
  vcore_saved_ucontext = NULL;  
  vcore_entry();
}

