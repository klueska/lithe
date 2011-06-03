#ifndef LITHE_TEST_H
#define LITHE_TEST_H

#include <lithe/lithe.h>
#include <lithe/fatal.h>

void yield_nyi(void *this, lithe_sched_t *child)
{
  fatal("should not be calling yield");
}

void reg_nyi(void *this, lithe_sched_t *child)
{
  fatal("should not be calling register");
}

void unreg_nyi(void *this, lithe_sched_t *child)
{
  fatal("should not be calling unregister");
}

void request_nyi(void *this, lithe_sched_t *child, int k)
{
  fatal("should not be calling request");
}

void unblock_nyi(void *this, lithe_task_t *task)
{
  fatal("should not be calling unblock");
}

#endif
