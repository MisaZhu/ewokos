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

#if defined(ARM_V7)
/* On ARMv7, map_page/unmap_page explicitly publish page-table lines to PoC.
 * The TTBR0 IRGN/RGN/S bits set the walk attributes; SCTLR.TRE only controls
 * TEX remapping and is not a switch for table-walk cacheability. */
extern void __dcache_clean_pou_range(uint32_t start, uint32_t end);
extern void __dcache_flush_poc_range(uint32_t start, uint32_t end);
extern void __invalidate_icache_all_is(void);
/* ARMv7 keeps ASIDs disabled: an address-space switch invalidates the local
 * TLB, while a mapping change broadcasts the invalidation. The two must not
 * be conflated; another core may keep running the same address space
 * without ever reloading TTBR. */
#endif

#ifdef KERNEL_SMP

inline void flush_dcache(void) {
    __flush_dcache_all();
}

inline void invalidate_dcache(void) {
    __invalidate_dcache_all();
}

inline void invalidate_icache_all(void) {
#if defined(__aarch64__) || defined(ARM_V7)
    /* The ELF may run on another core, so every core must drop stale
     * instructions cached from reused physical pages. */
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
#elif defined(ARM_V7)
    /* Bulk table construction and board-level page-table copies still rely
     * on a whole D-cache publish; mapping changes must be invalidated on all
     * cores. The single-page path already publishes its table lines and no
     * longer pays for a whole-cache clean. */
    flush_dcache();
    __invalidate_icache_all();
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
#elif defined(ARM_V7)
    __invalidate_icache_all(); /* UP: a local ICIALLU is enough */
#else
    /* arm32 v5/v6 UP keeps whole-cache maintenance inside flush_tlb() */
#endif
}

inline void flush_tlb(void) {
#if defined(__aarch64__)
    __flush_tlb(); /* see the SMP variant for why no D-cache work is needed */
#elif defined(ARM_V7)
    /* Same bulk-table publish semantics as SMP; on UP the assembly uses a
     * local TLBI. */
    flush_dcache();
    invalidate_icache_all();
    __flush_tlb();
#else
    flush_dcache();
    __flush_tlb();
#endif
}
#endif

/*
 * TLB-only invalidate for mappings whose descriptors are already visible to
 * the table walker. On ARMv7, map_page/unmap_page publish each changed
 * descriptor to PoC by VA (DCCIMVAC) and __flush_tlb()'s leading DSB orders
 * the invalidate after that clean, so no whole-D-cache sweep is needed. On
 * aarch64 the walk is PIPT-coherent and the DSB inside __flush_tlb() publishes
 * the PTE store, so flush_tlb_nosweep() is identical to flush_tlb() there. It
 * skips both the whole-D-cache clean+invalidate and the I-cache drop, so it is
 * valid only for data mappings (stack/heap/COW) that no non-coherent master
 * reads through a cacheable alias; the context-switch sweep still covers any
 * such alias. Bulk and board-level table copies, and the exec code path, keep
 * using flush_tlb(). Older ARM versions have no per-line publish and fall back.
 */
inline void flush_tlb_nosweep(void) {
#if defined(__aarch64__) || defined(ARM_V7)
    __flush_tlb();
#else
    flush_tlb();
#endif
}

/*
 * Make code the kernel just wrote (ELF load) visible to instruction fetch.
 * On aarch64 and ARMv7, ELF loading does not rely on side effects of TLB
 * maintenance: clean the code lines first, then invalidate the I-cache of
 * every running core. arm32 v5/v6 keep the original whole-cache path.
 */
inline void dcache_clean_code_range(const void* start, uint32_t size) {
#if defined(__aarch64__)
    if(size == 0)
        return;
    __dcache_clean_pou_range((uint64_t)start, (uint64_t)start + size);
#elif defined(ARM_V7)
    if(size == 0)
        return;
    __dcache_clean_pou_range((uint32_t)start, (uint32_t)start + size);
#else
    (void)start;
    (void)size;
#endif
}

/*
 * Push data the kernel wrote through a cacheable alias out to DRAM and drop
 * the lines, so a Non-Cacheable alias of the same frame (contig shm, dma)
 * or a non-coherent master reads what was written and no later eviction
 * overwrites what they wrote. aarch64 and ARMv7 do it by VA; arm32 v5/v6 still
 * clean the whole D-cache inside flush_tlb() and need nothing here.
 */
inline void dcache_flush_range(const void* start, uint32_t size) {
#if defined(__aarch64__)
    if(size == 0)
        return;
    __dcache_flush_poc_range((uint64_t)start, (uint64_t)start + size);
#elif defined(ARM_V7)
    if(size == 0)
        return;
    __dcache_flush_poc_range((uint32_t)start, (uint32_t)start + size);
#else
    (void)start;
    (void)size;
#endif
}

/*
 * Invalidate only the TLB entries of the affected page. On ARMv7,
 * map_page/unmap_page have already published the descriptor to PoC; on
 * aarch64 the walker uses cacheable, inner-shareable attributes and the DSB
 * before the TLBI publishes the PTE store. Both wait for completion and
 * synchronize context after the invalidation, so these single-page paths no
 * longer need a whole D-cache clean. Older ARM versions keep the full path.
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
#elif defined(ARM_V7)
    /* map_page/unmap_page have published the descriptor. The TLBI operand is
     * VA[31:12] in place, not addr >> 12 as on aarch64. The all-ASID form
     * also covers global pages. */
    ewokos_addr_t page = addr & ~(ewokos_addr_t)0xfff;
    __asm__ volatile(
        "dsb\n"
#ifdef KERNEL_SMP
        "mcr p15, 0, %0, c8, c3, 3\n" /* TLBIMVAAIS */
#else
        "mcr p15, 0, %0, c8, c7, 3\n" /* TLBIMVAA */
#endif
        "dsb\n"
        "isb\n"
        :: "r"(page) : "memory");
#elif defined(__arm__)
    /* ARMv5/v6 lack the ARMv7 table-line publish path, so keep the original
     * whole D-cache clean plus TLB invalidate; a bare TLBI would leave dirty
     * page tables unpublished. */
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
#if defined(ARM_V7)
    /* The new page table must be published before TTBR is loaded; the
     * assembly does the local TLB invalidate and ISB. The whole-cache publish
     * is kept to cover tables copied by clone_kernel_vm and directly by board
     * code. */
    flush_dcache();
    __invalidate_icache_all();
    __set_translation_table_base(tlb_base);
#else
    __set_translation_table_base(tlb_base);
    flush_tlb();
#endif
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
#elif defined(ARM_V7)
    /* Keep the current global-page scheme; do not re-enable the ASID-based
     * switch without TLB invalidation. */
    (void)asid;
    set_translation_table_base(tlb_base);
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
#elif defined(ARM_V7)
    /* Real-hardware fallback: ASID tagging disabled; a full flush covers all. */
    (void)asid;
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
