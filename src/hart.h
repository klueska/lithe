/* Copyright (c) 2011 The Regents of the University of California
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See COPYING for details.
 */

/**
 * Hart interface.
 */

#ifndef HART_H
#define HART_H

#include <parlib/vcore.h>

/**
 * Returns the id of the calling hart.
 */
static inline int hart_id(void)
{
	return vcore_id();
}

/**
 * Returns the current number of vcores allocated.
 */
static inline size_t num_harts(void)
{
	return num_vcores();
}

/**
 * Returns the maximum number of vcores requested.
 */
static inline size_t max_harts(void)
{
	return max_vcores();
}

/**
 * Returns the limit of allocatable vcores.
 */
static inline size_t limit_harts(void)
{
	return limit_vcores();
}

#define hart_set_tls_var(name, val) \
  vcore_set_tls_var(name, val)

#define hart_get_tls_var(name) \
  vcore_get_tls_var(name)

#endif
