#include <stdio.h>
#include <unistd.h>

#include <parlib/parlib.h>
#include <parlib/spinlock.h>
#include <src/lithe.hh>

using namespace lithe;

class TestScheduler : public Scheduler {
 protected:
  void hart_enter();

 public:
  TestScheduler();
  ~TestScheduler();
};

TestScheduler::TestScheduler()
{
  this->main_context = new lithe_context_t();
}

TestScheduler::~TestScheduler()
{
  delete this->main_context;
}

void TestScheduler::hart_enter()
{
  printf("enter() on hart %d\n", hart_id());
  lithe_hart_request(-1);
  lithe_hart_yield();
}

static void test_run()
{
  printf("TestScheduler Started!\n");
  size_t limit = max_harts();
  for(int i=0; i<100; i++) {
    printf("max_harts: %lu\n", limit);
    printf("num_harts: %lu\n", num_harts());
    printf("Requesting harts\n");
    lithe_hart_request(limit - 1);
    while (num_harts() > 1)
      cpu_relax();
    printf("Finished iteration %d\n", i);
  }
  printf("TestScheduler finishing!\n");
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

