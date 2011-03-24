#ifndef SPINLOCK_H
#define SPINLOCK_H

#ifdef __cplusplus
extern "C" {
#endif


#define LOCKED (1)
#define UNLOCKED (0)

int spinlock_init(int *lock);
int spinlock_trylock(int *lock);
int spinlock_lock(int *lock);
int spinlock_unlock(int *lock);


#ifdef __cplusplus
}
#endif


#endif // SPINLOCK_H
