/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

// Main public header file for our user-land support library,
// whose code lives in the lib directory.
// This library is roughly our OS's version of a standard C library,
// and is intended to be linked into all user-mode applications
// (NOT the kernel or boot loader).

#ifndef INC_PARLIB_H
#define INC_PARLIB_H 1

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <assert.h>
#include <parlib/atomic.h>
#include <parlib/arch.h>

enum {
	PG_RDONLY = 4,
	PG_RDWR   = 6,
};

enum {
	FALSE = 0,
	TRUE
};

#endif	// !ASSEMBLER

#endif	// !INC_PARLIB_H
