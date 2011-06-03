#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <wait.h>
#include <errno.h>
#include <stdbool.h>

static void sigalrm_handler(int sig) 
{
  /* Kill self if timed out. */
  if (sig == SIGALRM) {
    exit(EXIT_FAILURE);
  }
}

char * strsignal(int signum) 
{
  if (signum == SIGALRM) {
    return "Timed Out";
  } else if (signum == SIGSEGV) {
    return "Segfault";
  } 
  else
    return "Shouldn't Happen!";
}

int main(int argc, char **argv) 
{
  int test;
  for (test = 1; test < argc; test++) {

    /* Run test in a different process as to not kill/hang the test launcher. */
    pid_t child_pid = fork();
    
    /* Child process (test). */
    if (child_pid == 0) {
      /* Set up alarm signal handling. */
      struct sigaction sa_alarm;
      sa_alarm.sa_handler = sigalrm_handler;
      sigaction(SIGALRM, &sa_alarm, NULL);

      /* Reset timeout alarm to kill the test if it hangs. */
      alarm(10);

      /* Run test. */
      int status = execv(argv[test], argv+test);
      exit(status);
    }
    
    /* Parent process (test launcher). */
    else if (child_pid > 0) {
      
      /* Wait for the test to complete. */
      int status;
      while (waitpid(child_pid, &status, 0) == -1) {}
      
      /* Report test status. */
      if ((WIFEXITED(status) == true) && (WEXITSTATUS(status) == EXIT_SUCCESS)) {
	  printf("%-20s: SUCCESSFUL\n", argv[test]);
      } else {
	fprintf(stderr, "%-20s: FAILED %s\n", 
		argv[test], strsignal(WTERMSIG(status)));
      }
    }
    
    /* Fork error. */
    else {
      fprintf(stderr, "%-20s:\t UNEXECUTED (Could Not Fork)\n", argv[test]);
    }
  }

  exit(EXIT_SUCCESS);
}

