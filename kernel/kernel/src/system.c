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

#if defined(__aarch64__)
extern void __flush_tlb_asid(uint64_t asid);
extern void __flush_tlb_local(void);
extern void __set_translation_table_base_asid(uint64_t base, uint64_t asid);
extern void __dcache_clean_pou_range(uint64_t start, uint64_t end);
extern void __dcache_flush_poc_range(uint64_t start, uint64_t end);
extern void __invalidate_icache_all_is(void);

/*
 * ID_AA64MMFR0_EL1.ASIDBits: 0 = 8-bit, 2 = 16-bit. boot.S sets TCR.AS=1,
 * which is RES0 on 8-bit cores, so the usable ASID range must be checked
 * here rather than assumed - an ASID that does not fit falls back to the
 * flush-on-switch path instead of silently aliasing another space.
 */
static uint32_t _asid_limit = 0;

static inline uint32_t asid_limit(void) {
    if(_asid_limit == 0) {
        uint64_t mmfr0;
        __asm__ volatile("mrs %0, id_aa64mmfr0_el1" : "=r"(mmfr0));
        _asid_limit = (((mmfr0 >> 4) & 0xf) == 2) ? (1u << 16) : (1u << 8);
    }
    return _asid_limit;
}
#endif

#ifdef KERNEL_SMP

inline void flush_dcache(void) {
    __flush_dcache_all();
}

inline void invalidate_dcache(void) {
    __invalidate_dcache_all();
}

inline void invalidate_icache_all(void) {
#if defined(__aarch64__)
    __invalidate_icache_all_is();
#else
    __invalidate_icache_all();
#endif
}

inline void flush_tlb(void) {
#if defined(__aarch64__)
    /*
     * Page tables are walked through the D-cache (TCR IRGN/ORGN write-back,
     * inner shareable) and user memory is PIPT-coherent, so publishing PTE
     * stores only needs the dsb inside __flush_tlb. The historical whole
     * D-cache clean+invalidate by set/way (every L1/L2 line, on every
     * mapping change and every process switch) and the I-cache drop bought
     * nothing for TLB correctness; exec keeps its own code-coherency step
     * (dcache_clean_code_range + invalidate_icache_all).
     */
    __flush_tlb();
#else
    flush_dcache();
    invalidate_icache_all();
    __flush_tlb();
#endif
}

#else

inline void flush_dcache(void) { 
    __flush_dcache_all();
}

inline void invalidate_dcache(void) {
    __invalidate_dcache_all();
}

inline void invalidate_icache_all(void) {
#if defined(__aarch64__)
    __invalidate_icache_all_is();
#else
    /* arm32 UP keeps whole-cache maintenance inside flush_tlb() */
#endif
}

inline void flush_tlb(void) {
#if defined(__aarch64__)
    __flush_tlb(); /* see the SMP variant for why no D-cache work is needed */
#else
    flush_dcache();
    __flush_tlb();
#endif
}
#endif

/*
 * Make code the kernel just wrote (ELF load) visible to instruction fetch.
 * Only aarch64 needs an explicit step: its flush_tlb() no longer cleans the
 * whole D-cache, and the L1 I-cache does not snoop L1 D. Other archs still
 * run the full clean inside flush_tlb() right after the load.
 */
inline void dcache_clean_code_range(const void* start, uint32_t size) {
#if defined(__aarch64__)
    if(size == 0)
        return;
    __dcache_clean_pou_range((uint64_t)start, (uint64_t)start + size);
#else
    (void)start;
    (void)size;
#endif
}

/*
 * Push data the kernel wrote through a cacheable alias out to DRAM and drop
 * the lines, so a Non-Cacheable alias of the same frame (contig shm, dma)
 * or a non-coherent master reads what was written and no later eviction
 * overwrites what they wrote. aarch64 does it by VA; the other archs still
 * clean the whole D-cache inside flush_tlb() and need nothing here.
 */
inline void dcache_flush_range(const void* start, uint32_t size) {
#if defined(__aarch64__)
    if(size == 0)
        return;
    __dcache_flush_poc_range((uint64_t)start, (uint64_t)start + size);
#else
    (void)start;
    (void)size;
#endif
}

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
    /*
     * User pages are nG and tagged with the owning space's ASID, so use the
     * all-ASID form (VAAE1IS): the plain VAE1IS would only hit entries whose
     * ASID equals the (zero) top bits of the operand.
     */
    ewokos_addr_t page = addr >> 12; /* TLBI VA operand: VA[55:12], granule-independent */
    __asm__ volatile(
        "dsb ishst\n"
        "tlbi vaae1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"(page) : "memory");
#elif defined(__arm__)
    /* ARMv7 and older: __set_translation_table_base() programs TTBR0 with
     * IRGN/ORGN/S bits all zero, so the table walker fetches page tables as
     * non-cacheable memory and does NOT snoop the D-cache. PTE stores made
     * through the cacheable kernel mapping stay in the D-cache and a bare
     * dsb+TLBIMVAIS never publishes them to the walker (hangs real Cortex-A7
     * boards like miyoo; QEMU does not model this). Keep the historical full
     * flush (D-cache clean + global TLB invalidate) on arm32. */
    (void)addr;
    flush_tlb();
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

/*
 * Address-space switch. On aarch64 every space carries its own ASID and its
 * user PTEs are nG, so loading TTBR0 with base|ASID is the whole TLB side of
 * the switch - no TLBI. Entries of other spaces simply stop matching. Only a
 * space whose ASID does not fit the core's ASID width (or asid 0 = none)
 * takes the old route: local full TLB invalidate.
 *
 * DMA ordering and shared-device serialization belong at the driver's
 * handoff boundaries, not here. A whole-cache sweep on every switch destroys
 * the working set and must not substitute for a mailbox transaction lock or
 * a DMA/MMIO memory barrier.
 */
inline void set_translation_table_base_asid(ewokos_addr_t tlb_base, uint32_t asid) {
#if defined(__aarch64__)
    if(asid != 0 && asid < asid_limit()) {
        __set_translation_table_base_asid(tlb_base, asid);
        return;
    }
    __set_translation_table_base(tlb_base);
    __flush_tlb_local();
#else
    (void)asid;
    set_translation_table_base(tlb_base);
#endif
}

/*
 * Drop every TLB entry tagged with one ASID, on all cores. Used when a
 * space's ASID is (re)assigned so a recycled number never resolves through
 * the previous owner's leftovers. Other archs have no ASID tagging and their
 * switch-time flush already covers this.
 */
inline void flush_tlb_asid(uint32_t asid) {
#if defined(__aarch64__)
    if(asid != 0 && asid < asid_limit())
        __flush_tlb_asid(asid);
    else
        flush_tlb();
#else
    (void)asid;
#endif
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
