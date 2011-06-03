#include <stdio.h>
#include <unistd.h>

#include "lithe.hh"

using namespace lithe;

class TestScheduler : public Scheduler {
 private:
 protected:
  virtual void enter();
  virtual void yield(lithe_sched_t *child);
  virtual void reg(lithe_sched_t *child);
  virtual void unreg(lithe_sched_t *child);
  virtual void request(lithe_sched_t *child, int k);
  virtual void unblock(lithe_task_t *task);

 public:
  TestScheduler() {}
  ~TestScheduler() {}
};


void TestScheduler::enter()
{
  printf("0x%x: enter\n", (unsigned int)this);
  sleep(5);
  lithe_sched_yield();
}


void TestScheduler::yield(lithe_sched_t *child)
{
  printf("0x%x: yield\n", (unsigned int)this);
  lithe_sched_yield();
}


void TestScheduler::reg(lithe_sched_t *child)
{
  printf("0x%x: reg\n", (unsigned int)this);
}


void TestScheduler::unreg(lithe_sched_t *child)
{
  printf("0x%x: unreg\n", (unsigned int)this);
}


void TestScheduler::request(lithe_sched_t *child, int k)
{
  printf("0x%x: request\n", (unsigned int)this);
}


void TestScheduler::unblock(lithe_task_t *task)
{
  printf("0x%x: unblock\n", (unsigned int)this);
}


extern "C" {

int main(int argc, char **argv)
{
  printf("main start\n");
//   TestScheduler sched;
//   sched.doRegister();
//   sched.doRequest(1);
//   sleep(1);
//   sched.doUnregister();
  printf("main finish\n");
  return 0;
}
 
}
