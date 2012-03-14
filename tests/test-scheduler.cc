#include <stdio.h>
#include <unistd.h>

#include <parlib/parlib.h>
#include <src/lithe.hh>

using namespace lithe;

class TestScheduler : public Scheduler {
 protected:
  void hart_enter();

 public:
  unsigned int counter;
  TestScheduler() { counter = 0; }
  ~TestScheduler() {}
};

void TestScheduler::hart_enter()
{
  unsigned int *counter = &this->counter;
  printf("enter() on hart %d\n", hart_id());
  __sync_fetch_and_add(counter, 1);
  printf("counter: %d\n", *counter);
  lithe_hart_yield();
}

void test_run()
{
  printf("TestScheduler Starting!\n");
  TestScheduler *sched = (TestScheduler*)lithe_sched_current();
  unsigned int limit = limit_harts();
  if(limit == 1) {
    printf("ERROR: This simple test spins on one hart.\n");
    printf("       It requires at least 2 harts in order to run.\n");
    printf("       Other tests in this suite are more sophisticated and should run just fine.\n");
    printf("       Are you running this on a machine with only 1 CPU?\n");
    exit(1);
  }
  for(int i=0; i<100; i++) {
    unsigned int cur;
    do {
      cur = num_harts();
    } while(!(limit - cur));
    sched->counter = 0;
    printf("counter: %d\n", sched->counter);
    printf("limit_harts: %d\n", limit);
    printf("num_harts: %d\n", cur);
    printf("Requesting harts\n");
    lithe_hart_request(limit - cur);
    printf("Waiting for counter to reach: %d\n", (limit - cur));
    while(sched->counter < (limit - cur))
      cpu_relax();
    printf("All harts returned\n");
  }
  printf("TestScheduler Finishing!\n");
}

int main(int argc, char **argv)
{
  printf("CXX Lithe Simple test starting\n");
  TestScheduler test_sched;
  lithe_sched_enter(&test_sched);
  test_run();
  lithe_sched_exit();
  printf("CXX Lithe Simple test finishing\n");
  return 0;
}

