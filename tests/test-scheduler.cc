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
  spin_barrier_t barrier;
  TestScheduler();
  ~TestScheduler();
};

TestScheduler::TestScheduler()
{
  spin_barrier_init(&this->barrier, max_harts());
  this->main_context = new lithe_context_t();
}

TestScheduler::~TestScheduler()
{
  delete this->main_context;
}

void TestScheduler::hart_enter()
{
  printf("enter() on hart %d\n", hart_id());
  spin_barrier_wait(&this->barrier);
  spin_barrier_wait(&this->barrier);
  lithe_hart_yield();
}

static void test_run()
{
  printf("TestScheduler Started!\n");
  TestScheduler *sched = (TestScheduler*)lithe_sched_current();
  size_t limit = max_harts();
  for(int i=0; i<100; i++) {
    printf("max_harts: %lu\n", limit);
    printf("num_harts: %lu\n", num_harts());
    printf("Requesting harts\n");
    lithe_hart_request(limit);
    spin_barrier_wait(&sched->barrier);
    lithe_hart_request(1);
    spin_barrier_wait(&sched->barrier);
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

