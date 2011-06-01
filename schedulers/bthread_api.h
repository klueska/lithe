/*
 * Copyright (c) 2011 The Regents of the University of California
 * Barret Rhoden <brho@cs.berkeley.edu>
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#ifndef _BTHREAD_API_H
#define _BTHREAD_API_H

int bthread_attr_init(bthread_attr_t *);
int bthread_attr_destroy(bthread_attr_t *);
int bthread_create(bthread_t *, const bthread_attr_t *,
                   void *(*)(void *), void *);
int bthread_join(bthread_t, void **);
int bthread_yield(void);

int bthread_attr_setdetachstate(bthread_attr_t *__attr,int __detachstate);

int bthread_mutex_destroy(bthread_mutex_t *);
int bthread_mutex_init(bthread_mutex_t *, const bthread_mutexattr_t *);
int bthread_mutex_lock(bthread_mutex_t *);
int bthread_mutex_trylock(bthread_mutex_t *);
int bthread_mutex_unlock(bthread_mutex_t *);
int bthread_mutex_destroy(bthread_mutex_t *);

int bthread_mutexattr_init(bthread_mutexattr_t *);
int bthread_mutexattr_destroy(bthread_mutexattr_t *);
int bthread_mutexattr_gettype(const bthread_mutexattr_t *, int *);
int bthread_mutexattr_settype(bthread_mutexattr_t *, int);

int bthread_cond_init(bthread_cond_t *, const bthread_condattr_t *);
int bthread_cond_destroy(bthread_cond_t *);
int bthread_cond_broadcast(bthread_cond_t *);
int bthread_cond_signal(bthread_cond_t *);
int bthread_cond_wait(bthread_cond_t *, bthread_mutex_t *);

int bthread_condattr_init(bthread_condattr_t *);
int bthread_condattr_destroy(bthread_condattr_t *);
int bthread_condattr_setpshared(bthread_condattr_t *, int);
int bthread_condattr_getpshared(bthread_condattr_t *, int *);

int bthread_rwlock_destroy(bthread_mutex_t *);
int bthread_rwlock_init(bthread_mutex_t *, const bthread_mutexattr_t *);
int bthread_rwlock_unlock(bthread_mutex_t *);
int bthread_rwlock_rdlock(bthread_mutex_t *);
int bthread_rwlock_wrlock(bthread_mutex_t *);
int bthread_rwlock_tryrdlock(bthread_mutex_t *);
int bthread_rwlock_trywrlock(bthread_mutex_t *);

bthread_t bthread_self();
int bthread_equal(bthread_t t1, bthread_t t2);
void bthread_exit(void* ret);
int bthread_once(bthread_once_t* once_control, void (*init_routine)(void));

int bthread_barrier_init(bthread_barrier_t* b, const bthread_barrierattr_t* a, int count);
int bthread_barrier_wait(bthread_barrier_t* b);
int bthread_barrier_destroy(bthread_barrier_t* b);

int bthread_detach(bthread_t __th);
int bthread_attr_setstacksize(bthread_attr_t *attr, size_t stacksize);
int bthread_attr_getstacksize(const bthread_attr_t *attr, size_t *stacksize);

#endif
