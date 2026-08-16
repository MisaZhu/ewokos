#include <kernel/system.h>
#include <kernel/core.h>
#include <dev/timer.h>

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

extern void __flush_dcache_all(void);
extern void __invalidate_dcache_all(void);
extern void __invalidate_icache_all(void);

#ifdef KERNEL_SMP

inline void flush_dcache(void) {
    __flush_dcache_all();
}

inline void invalidate_dcache(void) {
    __invalidate_dcache_all();
}

inline void invalidate_icache_all(void) {
    __invalidate_icache_all();
}

inline void flush_tlb(void) {
    flush_dcache();
    invalidate_icache_all();
    __flush_tlb();
}

#else

inline void flush_dcache(void) { 
    __flush_dcache_all();
}

inline void invalidate_dcache(void) {
    __invalidate_dcache_all();
}

inline void flush_tlb(void) {
    flush_dcache();
    __flush_tlb();
}
#endif

/*
 * Range-scoped TLB invalidation for a single page.
 *
 * map_page()/unmap_page() write PTEs but emit no barriers of their own; the
 * global flush_tlb() (flush_dcache() + __flush_tlb + dsb) used to both publish
 * the PTE store and invalidate every TLB entry. For shm map/unmap we only touch
 * a handful of pages, so a full flush (whole D-cache clean + broadcast
 * invalidate of the entire TLB) is wasteful and adds cross-core TLB pressure.
 *
 * Invalidate just the affected VA instead. The dsb before the TLBI publishes
 * the PTE write to the walker (page tables are cacheable/coherent in the inner-
 * shareable domain), so the separate whole-D-cache clean is not needed on the
 * archs handled here. Unknown archs fall back to the safe global flush_tlb().
 */
inline void flush_tlb_addr(ewokos_addr_t addr) {
#if defined(__aarch64__)
    ewokos_addr_t page = addr >> 12; /* TLBI VA operand: VA[55:12], granule-independent */
    __asm__ volatile(
        "dsb ishst\n"
        "tlbi vae1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"(page) : "memory");
#elif defined(__arm__)
    /* ARMv7 SMP: TLBIMVAIS (invalidate unified TLB by MVA, inner shareable). */
    __asm__ volatile(
        "dsb ishst\n"
        "mcr p15, 0, %0, c8, c3, 1\n"
        "dsb ish\n"
        "isb\n"
        :: "r"(addr & ~(ewokos_addr_t)0xfff) : "memory");
#elif defined(__riscv)
    __asm__ volatile("sfence.vma %0" :: "r"(addr) : "memory");
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ volatile("invlpg (%0)" :: "r"(addr) : "memory");
#else
    (void)addr;
    flush_tlb();
#endif
}

inline void set_translation_table_base(ewokos_addr_t tlb_base) {
    __set_translation_table_base(tlb_base);
    flush_tlb();
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

inline void wfi(void) {
#ifdef ARM_V6
    __asm__("MOV r0, #0; MCR p15,0,R0,c7,c0,4");
#elif defined(__x86_64__)
    __asm__ volatile("hlt");
#else
    __asm__("WFI");
#endif
}

inline void halt(void) {
    while(1) {
        wfi();
    }
}
