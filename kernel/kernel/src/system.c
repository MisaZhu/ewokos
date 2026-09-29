#include <kernel/system.h>
#include <kernel/core.h>
#include <dev/timer.h>

/*
 * Platform-independent helpers only. Every cache / TLB / address-space
 * maintenance primitive (flush_tlb*, dcache_*, invalidate_*cache*,
 * set_translation_table_base*, wfi, ...) is implemented per-architecture under
 * kernel/platform/<arch>/arch/<ver>/system_arch.c so this shared file carries
 * no per-arch conditional compilation.
 */

void __attribute__((optimize("O0"))) _delay(uint32_t count) {
    while(count > 0) {
        count--;
    }
}

void _delay_usec(uint64_t count) {
    uint64_t s = timer_read_sys_usec();
    uint64_t t = s + count;
    while(s < t) {
        s = timer_read_sys_usec();
    }
}

inline void _delay_msec(uint32_t count) {
    _delay_usec(count*1000);
}

inline void set_vector_table(ewokos_addr_t vector) {
    __set_vector_table(vector);
}

#ifdef KERNEL_SMP
static int32_t _spin = 0;
static int32_t _klock = 0;
static int32_t _klock_owner = -1;

inline void kernel_lock_init(void) {
    _spin = _klock = 0;
    _klock_owner = -1;
}

inline int32_t kernel_lock_check(void) {
    return (_klock != 0 && _klock_owner == (int32_t)get_core_id()) ? 1 : 0;
}

inline void kernel_lock(void) {
    mcore_lock(&_spin);
    _klock_owner = (int32_t)get_core_id();
    _klock = 1;
}

inline void kernel_unlock(void) {
    _klock = 0;
    _klock_owner = -1;
    mcore_unlock(&_spin);
}
#endif

inline void halt(void) {
    while(1) {
        wfi();
    }
}
