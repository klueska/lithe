#include <stdio.h>
#include <unistd.h>

#include <lithe/lithe.hh>
#include <ht/atomic.h>

using namespace lithe;

class TestScheduler : public Scheduler {
 protected:
  void start(void *arg);
  void vcore_enter();

 public:
  unsigned int counter;
  TestScheduler() { counter = 0; }
  ~TestScheduler() {}
};

void TestScheduler::vcore_enter()
{
  unsigned int *counter = &this->counter;
  printf("enter() on vcore %d\n", vcore_id());
  fetch_and_add(counter, 1);
  printf("counter: %d\n", *counter);
  lithe_vcore_yield();
}

void TestScheduler::start(void *arg)
{
  printf("TestScheduler Started!\n");
  unsigned int *counter = &this->counter;
  for(int i=0; i<100; i++) {
    unsigned int limit, cur;
    do {
      limit = max_vcores();
      cur = num_vcores();
    } while(!(limit - cur));
    *counter = 0;
    printf("counter: %d\n", *counter);
    printf("max_vcores: %d\n", limit);
    printf("num_vcores: %d\n", cur);
    printf("Wait for counter to reach: %d\n", (limit - cur));
    printf("Requesting vcores\n");
    lithe_vcore_request(limit - cur);
    while (coherent_read(*counter) < (limit - cur));
    printf("All vcores returned\n");
  }
  printf("TestScheduler finishing!\n");
}

int main(int argc, char **argv)
{
  printf("CXX Lithe Simple test starting\n");
  TestScheduler sched;
  lithe_sched_start(&Scheduler::funcs, &sched, NULL);
  printf("CXX Lithe Simple test finishing\n");
  return EXIT_SUCCESS;
}

