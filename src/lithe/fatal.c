#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef DEBUG

void fatal(char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  fflush(stderr);
  va_end(args);
  exit(1);
}

void fatalerror(char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, ": ");
  perror(NULL);
  fflush(stderr);
  va_end(args);
  exit(1);
}

#else

void __fatal(char *file, int line, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, " (%s:%u)\n", file, line);
  fflush(stderr);
  va_end(args);
  exit(1);
}

void __fatalerror(char *file, int line, char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, " (%s:%u): ", file, line);
  perror(NULL);
  fflush(stderr);
  va_end(args);
  exit(1);
}

#endif // DEBUG
