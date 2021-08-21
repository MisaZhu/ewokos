#include "kernel/spinlock.h"

void spin_lock(spin_lock_t *spinlock) {
    if (atomic_get(&spinlock->lock) == 0) {
        atomic_set(&spinlock->lock, 1);
        __asm volatile("cpsid i");
    } else {
        __asm volatile("WFE");
    }
}

void spin_unlock(spin_lock_t *spinlock) {
    atomic_set(&spinlock->lock, 0);
    __asm volatile("cpsie i");
    __asm volatile("SEV");
}
