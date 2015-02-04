/* Copyright (c) 2011 The Regents of the University of California
 * Ben Hindman <benh@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Basic perror + exit routines.
 */

#ifndef FATAL_H
#define FATAL_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif 

#ifndef DEBUG

/*
 * Prints the string specified by 'fmt' to standard error before
 * calling exit.
 */
void fatal(const char *fmt, ...) __attribute__((noreturn));

/*
 * Prints the string specified by 'fmt' to standard error along with a
 * string containing the error number as specfied in 'errno' before
 * calling exit.
 */
void fatalerror(const char *fmt, ...) __attribute__((noreturn));

#else

/*
 * Like the non-debug version except includes the file name and line
 * number in the output.
 */
#define fatal(fmt...) __fatal(__FILE__, __LINE__, fmt)
void __fatal(const char *file, int line, const char *fmt, ...)
__attribute__((noreturn));

/*
 * Like the non-debug version except includes the file name and line
 * number in the output.
 */
#define fatalerror(fmt...) __fatalerror(__FILE__, __LINE__, fmt)
void __fatalerror(const char *file, int line, const char *fmt, ...)
__attribute__((noreturn));

#endif // DEBUG

#ifdef __cplusplus
}
#endif 

#endif // FATAL_H
