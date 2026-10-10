#ifndef EWOKSYS_SPINLOCK_H
#define EWOKSYS_SPINLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TTAS spinlock for short critical sections in userspace SMP services.
   pthread_mutex sleeps on contention and relies on an IPI for cross-core
   wakeup; when the protected region is only a table scan or a counter
   bump, the IPI round-trip dominates the actual hold time. A spinlock
   avoids the kernel entirely.

   __atomic_test_and_set maps to LDAXR/STXR (aarch64), LDREX/STREX (arm),
   XCHG (x86). The inner relaxed-load loop keeps the cache line in Shared
   state so the unlock store propagates in one coherence miss.

   Usage:
       static spinlock_t my_lock = SPINLOCK_INIT;
       spin_lock(&my_lock);
       ... short critical section ...
       spin_unlock(&my_lock);

   Do NOT hold a spinlock across blocking calls (shmat, syscall that may
   park, etc.). For longer waits use sched_yield() outside the lock. */

typedef volatile int32_t spinlock_t;

#define SPINLOCK_INIT 0

static inline void spin_lock(spinlock_t* s) {
	while(__atomic_test_and_set(s, __ATOMIC_ACQUIRE)) {
		while(__atomic_load_n(s, __ATOMIC_RELAXED)) {
#if defined(__aarch64__) || (defined(__arm__) && __ARM_ARCH >= 7)
			__asm volatile("yield" ::: "memory");
#elif defined(__i386__) || defined(__x86_64__)
			__asm volatile("pause" ::: "memory");
#endif
		}
	}
}

static inline void spin_unlock(spinlock_t* s) {
	__atomic_clear(s, __ATOMIC_RELEASE);
}

/* try_lock: non-blocking acquire, returns 1 on success, 0 if held */
static inline int spin_trylock(spinlock_t* s) {
	return !__atomic_test_and_set(s, __ATOMIC_ACQUIRE);
}

#ifdef __cplusplus
}
#endif

#endif /* EWOKSYS_SPINLOCK_H */
