/*
 * ARMv7-A cache / TLB / address-space maintenance.
 *
 * Platform-specific implementation of the interfaces declared in
 * <kernel/system.h>. Reproduces the branch that used to live behind
 * `#elif defined(ARM_V7)` in the common kernel/kernel/src/system.c.
 *
 * On ARMv7, map_page/unmap_page explicitly publish page-table lines to PoC and
 * the walk is cacheable/inner-shareable (see v7/system.S + v7/boot.S), so PTE
 * visibility does not depend on a whole-cache sweep for single-page changes.
 */
#include <kernel/system.h>
#include <kernel/core.h>

extern void __flush_dcache_all(void);
extern void __invalidate_dcache_all(void);
extern void __invalidate_icache_all(void);
extern void __invalidate_icache_all_is(void);
extern void __flush_tlb(void);
extern void __flush_tlb_asid(uint32_t asid);
extern void __set_translation_table_base(ewokos_addr_t base);
extern void __set_translation_table_base_asid(ewokos_addr_t base, uint32_t asid);
extern void __dcache_clean_pou_range(uint32_t start, uint32_t end);
extern void __dcache_flush_poc_range(uint32_t start, uint32_t end);

/* CONTEXTIDR.ASID is 8 bits on ARMv7-A short-descriptor translation and
 * proc_space_asid() = pde_index+1 reserves 0, so usable ASIDs are 1..255. A
 * space whose ASID does not fit falls back to the full-TLBIALL switch instead
 * of silently truncating and aliasing another space. */
static inline uint32_t asid_limit(void) { return 256; }

void flush_dcache(void) {
    __flush_dcache_all();
}

void invalidate_dcache(void) {
    __invalidate_dcache_all();
}

void invalidate_icache_all(void) {
#ifdef KERNEL_SMP
    /* The ELF may run on another core, so every core must drop stale
     * instructions cached from reused physical pages. */
    __invalidate_icache_all_is();
#else
    __invalidate_icache_all(); /* UP: a local ICIALLU is enough */
#endif
}

void flush_tlb(void) {
    /* Bulk table construction and board-level page-table copies still rely
     * on a whole D-cache publish; mapping changes must be invalidated on all
     * cores. The single-page path already publishes its table lines and no
     * longer pays for a whole-cache clean. */
    flush_dcache();
    __invalidate_icache_all();
    __flush_tlb();
}

/* See the flush_tlb_nosweep() contract in <kernel/system.h>: descriptors are
 * already published to PoC by map_page/unmap_page and __flush_tlb()'s leading
 * DSB orders the invalidate after that clean, so no whole-D-cache sweep. */
void flush_tlb_nosweep(void) {
    __flush_tlb();
}

void dcache_clean_code_range(const void* start, uint32_t size) {
    if(size == 0)
        return;
    __dcache_clean_pou_range((uint32_t)start, (uint32_t)start + size);
}

void dcache_flush_range(const void* start, uint32_t size) {
    if(size == 0)
        return;
    __dcache_flush_poc_range((uint32_t)start, (uint32_t)start + size);
}

void flush_tlb_addr(ewokos_addr_t addr) {
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
}

void set_translation_table_base(ewokos_addr_t tlb_base) {
    /* The new page table must be published before TTBR is loaded; the
     * assembly does the local TLB invalidate and ISB. The whole-cache publish
     * is kept to cover tables copied by clone_kernel_vm and directly by board
     * code. */
    flush_dcache();
    __invalidate_icache_all();
    __set_translation_table_base(tlb_base);
}

/*
 * Address-space switch. See the common contract in <kernel/system.h>.
 */
void set_translation_table_base_asid(ewokos_addr_t tlb_base, uint32_t asid) {
    if(asid != 0 && asid < asid_limit()) {
        /* Keep the switch-time whole-D-cache sweep: on ARMv7 it is load-bearing
         * for non-coherent DMA/graphics doorbell ordering (removing it freezes
         * X on real boards; an I-cache-only or bare-dsb variant does not
         * substitute). The ASID write replaces the per-switch TLBIALL/BPIALL so
         * each space keeps its own warm, ASID-tagged user TLB entries. */
        flush_dcache();
        __set_translation_table_base_asid(tlb_base, asid);
        return;
    }
    /* ASID does not fit the 8-bit CONTEXTIDR field: take the full-invalidate
     * switch (CONTEXTIDR=0 + TLBIALL), which also stops shared-ASID-0 fallback
     * spaces from colliding in the TLB. */
    set_translation_table_base(tlb_base);
}

void flush_tlb_asid(uint32_t asid) {
    /* Evict a recycled ASID's tagged entries on all cores (TLBIASIDIS) so a
     * reused pde slot never resolves through the previous owner. An out-of-
     * range ASID uses the full flush, which covers every ASID. */
    if(asid != 0 && asid < asid_limit())
        __flush_tlb_asid(asid);
    else
        flush_tlb();
}

void wfi(void) {
    __asm__("WFI");
}
