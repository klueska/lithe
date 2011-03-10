#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <ht.h>

void entry()
{
  printf("entry\n");
  ht_yield();
}


int main()
{
  printf("main\n");
  ht_request_async(1);
  sleep(5);
  return 0;
}
