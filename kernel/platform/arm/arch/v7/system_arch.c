/*
 * ARMv7-A cache / TLB / address-space maintenance.
 *
 * Platform-specific implementation of the interfaces declared in
 * <kernel/system.h>. Reproduces the branch that used to live behind
 * `#elif defined(ARM_V7)` in the common kernel/kernel/src/system.c.
 *
 * On ARMv7 the table walk is Non-cacheable (TTBR0 IRGN/RGN=0, see v7/boot.S
 * and __set_translation_table_base in v7/system.S), so map_page/unmap_page
 * explicitly publish every page-table line to PoC with dcache_flush_range and
 * PTE visibility does not depend on a whole-cache sweep for single-page changes.
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
    /*
     * ARMv7 walks page tables as non-cacheable memory (TTBR0 IRGN/ORGN/S=0,
     * see v7/boot.S and __set_translation_table_base), so the walker does NOT
     * snoop the D-cache. A PTE store made through the cacheable kernel mapping
     * stays in the D-cache, and a bare dsb + scoped TLBIMVAA here never
     * publishes it to the walker -> intermittent translation faults during
     * early boot (shm.c map/unmap, proc_load_elf). On real Cortex-A7 (miyoo)
     * this is the pre-logo black screen that only some power-on boots hit;
     * QEMU virt arm32 does not model walker/D-cache incoherence so it never
     * reproduces. Keep the historical full flush (whole-D-cache clean to PoC +
     * global TLB invalidate) -- this is the configuration verified to boot on
     * the board (matches the pre-optimization arm32 flush_tlb_addr). Do NOT
     * re-optimize to a scoped TLBI without re-validating on hardware.
     */
    (void)addr;
    flush_tlb();
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
 *
 * ARMv7 keeps the global-page scheme: every switch is the full
 * TTBR0 + CONTEXTIDR=0 + TLBIALL + BPIALL sequence behind a whole-D-cache
 * clean. The ASID-tagged switch without TLB invalidation
 * (__set_translation_table_base_asid) does not boot reliably on the real
 * Cortex-A7 boards (miyoo: intermittent black screen on power-up), while this
 * sequence is the one verified on hardware. Do not re-enable the ASID path
 * here without re-validating on a board.
 */
void set_translation_table_base_asid(ewokos_addr_t tlb_base, uint32_t asid) {
    (void)asid;
    set_translation_table_base(tlb_base);
}

void flush_tlb_asid(uint32_t asid) {
    /* Global-page scheme: there are no ASID-private entries to evict, a
     * recycled pde slot is covered by the full flush on all cores. */
    (void)asid;
    flush_tlb();
}

void wfi(void) {
    __asm__("WFI");
}
