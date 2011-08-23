#include <stdio.h>
#include <unistd.h>

#include <lithe/lithe.hh>
#include <ht/atomic.h>

using namespace lithe;

class TestScheduler : public Scheduler {
 protected:
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

void test_run()
{
  printf("TestScheduler Starting!\n");
  TestScheduler *sched = (TestScheduler*)lithe_sched_current();
  for(int i=0; i<100; i++) {
    unsigned int limit, cur;
    do {
      limit = max_vcores();
      cur = num_vcores();
    } while(!(limit - cur));
    sched->counter = 0;
    printf("counter: %d\n", sched->counter);
    printf("max_vcores: %d\n", limit);
    printf("num_vcores: %d\n", cur);
    printf("Wait for counter to reach: %d\n", (limit - cur));
    printf("Requesting vcores\n");
    lithe_vcore_request(limit - cur);
    while (coherent_read(sched->counter) < (limit - cur));
    printf("All vcores returned\n");
  }
  printf("TestScheduler Finishing!\n");
}

int main(int argc, char **argv)
{
  printf("CXX Lithe Simple test starting\n");
  TestScheduler test_sched;
  lithe_context_t *context = __lithe_context_create_default(false);
  lithe_sched_enter(&Scheduler::funcs, &test_sched, context);
  test_run();
  lithe_sched_exit();
  __lithe_context_destroy_default(context, false);
  printf("CXX Lithe Simple test finishing\n");
  return 0;
}

