/*
 * aarch64 (ARMv8) cache / TLB / address-space maintenance.
 *
 * Platform-specific implementation of the interfaces declared in
 * <kernel/system.h>. Kept out of the common kernel/kernel/src/system.c so the
 * shared code carries no per-arch #ifdef. Every function here reproduces the
 * aarch64 branch that used to live behind `#if defined(__aarch64__)`.
 */
#include <kernel/system.h>
#include <kernel/core.h>

extern void __flush_dcache_all(void);
extern void __invalidate_dcache_all(void);
extern void __flush_tlb(void);
extern void __flush_tlb_asid(uint64_t asid);
extern void __flush_tlb_local(void);
extern void __set_translation_table_base(uint64_t base);
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

void flush_dcache(void) {
    __flush_dcache_all();
}

void invalidate_dcache(void) {
    __invalidate_dcache_all();
}

void invalidate_icache_all(void) {
    /* The ELF may run on another core, so every core must drop stale
     * instructions cached from reused physical pages. */
    __invalidate_icache_all_is();
}

void flush_tlb(void) {
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
}

/* See the flush_tlb_nosweep() contract in <kernel/system.h>: on aarch64 the
 * walk is PIPT-coherent and the DSB inside __flush_tlb() publishes the PTE
 * store, so the nosweep form is identical to flush_tlb(). */
void flush_tlb_nosweep(void) {
    __flush_tlb();
}

void dcache_clean_code_range(const void* start, uint32_t size) {
    if(size == 0)
        return;
    __dcache_clean_pou_range((uint64_t)start, (uint64_t)start + size);
}

void dcache_flush_range(const void* start, uint32_t size) {
    if(size == 0)
        return;
    __dcache_flush_poc_range((uint64_t)start, (uint64_t)start + size);
}

void flush_tlb_addr(ewokos_addr_t addr) {
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
}

void set_translation_table_base(ewokos_addr_t tlb_base) {
    __set_translation_table_base(tlb_base);
    flush_tlb();
}

/*
 * Address-space switch. Every space carries its own ASID and its user PTEs are
 * nG, so loading TTBR0 with base|ASID is the whole TLB side of the switch - no
 * TLBI. Entries of other spaces simply stop matching. Only a space whose ASID
 * does not fit the core's ASID width (or asid 0 = none) takes the old route:
 * local full TLB invalidate.
 */
void set_translation_table_base_asid(ewokos_addr_t tlb_base, uint32_t asid) {
    if(asid != 0 && asid < asid_limit()) {
        __set_translation_table_base_asid(tlb_base, asid);
        return;
    }
    __set_translation_table_base(tlb_base);
    __flush_tlb_local();
}

void flush_tlb_asid(uint32_t asid) {
    if(asid != 0 && asid < asid_limit())
        __flush_tlb_asid(asid);
    else
        flush_tlb();
}

void wfi(void) {
    __asm__("WFI");
}
