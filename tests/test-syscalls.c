#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <assert.h>
#include "src/mutex.h"
#include "src/fork_join_sched.h"

lithe_mutex_t mutex = LITHE_MUTEX_INITIALIZER(mutex);

void work(void *arg)
{
  int id = (int)(long)arg;
  const char str[] = "This is a test of the emergency broadcast system.  This is only a test";

  /* Setup the file. */
  char filename[20];
  sprintf(filename, "%d.dat", id);

  /* Write the file. */
  int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
  assert(fd != -1);
  int n = write(fd, str, strlen(str));
  assert(n == strlen(str));
  assert(close(fd) == 0);

  /* Read the file. */
  fd = open(filename, O_RDONLY);
  assert(fd != -1);
  char buf[sizeof(str)];
  assert(read(fd, buf, sizeof(buf)-1) == sizeof(buf)-1);
  buf[sizeof(buf)-1] = 0;
  assert(memcmp(buf, str, sizeof(buf)) == 0);
  assert(close(fd) == 0);

  /* Remove the file. */
  unlink(filename);

  /* Finish up. */
  lithe_mutex_lock(&mutex);
    printf("context %p done with syscalls (count = %d)\n", 
      lithe_context_self(), id);
  lithe_mutex_unlock(&mutex);
}

void root_run(int count)
{
  lithe_fork_join_sched_t *sched = (lithe_fork_join_sched_t*)lithe_sched_current();
  for(int i=0; i < count; i++) {
    lithe_fork_join_context_create(sched, 4096, work, (void*)(long)i);
  }
  lithe_fork_join_sched_join_all(sched);
}

int main(int argc, char **argv)
{
  printf("main start\n");
  lithe_fork_join_sched_t *sched = lithe_fork_join_sched_create();
  lithe_sched_enter(&sched->sched);
  root_run(3500);
  lithe_sched_exit(&sched->sched);
  printf("main finish\n");
  return 0;
}

