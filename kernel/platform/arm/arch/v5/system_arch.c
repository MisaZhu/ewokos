/*
 * ARMv5 (legacy arm32) cache / TLB / address-space maintenance.
 *
 * Platform-specific implementation of the interfaces declared in
 * <kernel/system.h>. Reproduces the `#else` (non-aarch64, non-ARM_V7) branch
 * that used to live in the common kernel/kernel/src/system.c.
 *
 * ARMv5/v6 lack the ARMv7 per-line table publish path: the walk is
 * Non-cacheable, so a bare TLBI would leave dirty page tables unpublished and
 * flush_tlb() keeps the original whole D-cache clean. The range-based clean /
 * flush helpers are no-ops here.
 */
#include <kernel/system.h>
#include <kernel/core.h>

extern void __flush_dcache_all(void);
extern void __invalidate_dcache_all(void);
extern void __flush_tlb(void);
extern void __set_translation_table_base(ewokos_addr_t base);
#ifdef KERNEL_SMP
extern void __invalidate_icache_all(void);
#endif

void flush_dcache(void) {
    __flush_dcache_all();
}

void invalidate_dcache(void) {
    __invalidate_dcache_all();
}

void invalidate_icache_all(void) {
#ifdef KERNEL_SMP
    __invalidate_icache_all();
#else
    /* arm32 v5/v6 UP keeps whole-cache maintenance inside flush_tlb() */
#endif
}

void flush_tlb(void) {
#ifdef KERNEL_SMP
    flush_dcache();
    invalidate_icache_all();
    __flush_tlb();
#else
    flush_dcache();
    __flush_tlb();
#endif
}

/* No per-line publish on this platform: fall back to the full flush_tlb(). */
void flush_tlb_nosweep(void) {
    flush_tlb();
}

void dcache_clean_code_range(const void* start, uint32_t size) {
    (void)start;
    (void)size;
}

void dcache_flush_range(const void* start, uint32_t size) {
    (void)start;
    (void)size;
}

void flush_tlb_addr(ewokos_addr_t addr) {
    /* ARMv5/v6 lack the ARMv7 table-line publish path, so keep the original
     * whole D-cache clean plus TLB invalidate; a bare TLBI would leave dirty
     * page tables unpublished. */
    (void)addr;
    flush_tlb();
}

void set_translation_table_base(ewokos_addr_t tlb_base) {
    __set_translation_table_base(tlb_base);
    flush_tlb();
}

void set_translation_table_base_asid(ewokos_addr_t tlb_base, uint32_t asid) {
    (void)asid;
    set_translation_table_base(tlb_base);
}

void flush_tlb_asid(uint32_t asid) {
    (void)asid;
}

void wfi(void) {
#ifdef ARM_V6
    __asm__("MOV r0, #0; MCR p15,0,R0,c7,c0,4");
#else
    __asm__("WFI");
#endif
}
