/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef SPINLOCK_H
#define SPINLOCK_H

#ifdef __cplusplus
extern "C" {
#endif


#define LOCKED (1)
#define UNLOCKED (0)

typedef int spinlock_t;

void spinlock_init(spinlock_t *lock);
int spinlock_trylock(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);


#ifdef __cplusplus
}
#endif


#endif // SPINLOCK_H
