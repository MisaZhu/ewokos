#ifndef __KERNEL_SPINLOCK_H__
#define __KERNEL_SPINLOCK_H__

#include "kernel/atomic.h"

#define spin_lock_create()                             \
    {                                                \
        .lock = {                                    \
                .counter = 0,                        \
        },                                           \
        .operations = {                              \
                .acquire = spinlock_default_acquire, \
                .release = spinlock_default_release, \
        },                                           \
    }

typedef void (*spin_lock_acquire)(struct st_spin_lock *spin_lock);

typedef void (*spin_lock_release)(struct st_spin_lock *spin_lock);

typedef struct spin_lock_operations {
    spin_lock_acquire acquire;
    spin_lock_release release;
} spin_lock_operations;

typedef struct st_spin_lock {
    atomic_t lock;
    spin_lock_operations operations;
} spin_lock_t;

void spin_lock(spin_lock_t *spinlock);

void spin_unlock(spin_lock_t *spinlock);

#endif// __KERNEL_SPINLOCK_H__
