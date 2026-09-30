/*
 * ARMv5 (legacy arm32) cache / TLB / address-space maintenance.
 *
 * Platform-specific implementation of the interfaces declared in
 * <kernel/system.h>. Reproduces the `#else` (non-aarch64, non-ARM_V7) branch
 * that used to live in the common kernel/kernel/src/system.c.
 *
 * ARMv5/v6 lack the ARMv7 per-line table publish path: the walk is
 * Non-cacheable, so a bare TLBI would leave dirty page tables unpublished and
 * flush_tlb() keeps the original whole D-cache clean.
 *
 * ARM926 caches are VIVT: a line filled through the kernel P2V alias is not
 * visible to a load at the user VA of the same page and vice versa. Every
 * table change and every address-space switch therefore does a full
 * clean+invalidate, and the range helpers below clean by the alias MVA so the
 * exec path does not have to rely on an incidental sweep further down.
 */
#include <kernel/system.h>
#include <kernel/core.h>

extern void __flush_dcache_all(void);
extern void __invalidate_dcache_all(void);
extern void __invalidate_icache_all(void);
extern void __dcache_clean_range(ewokos_addr_t start, ewokos_addr_t end);
extern void __dcache_flush_range(ewokos_addr_t start, ewokos_addr_t end);
extern void __flush_tlb(void);
extern void __set_translation_table_base(ewokos_addr_t base);

void flush_dcache(void) {
    __flush_dcache_all();
}

void invalidate_dcache(void) {
    __invalidate_dcache_all();
}

void invalidate_icache_all(void) {
    /*
     * Called once at the end of proc_load_elf. The loader cleaned only the
     * executable segments by range; the RW segments it wrote through the
     * kernel alias still sit dirty under alias VAs where user-VA loads never
     * hit. Push the whole D-cache out first so the image is complete in
     * memory even when the exec'ing process resumes without a table switch.
     */
    __flush_dcache_all();
    __invalidate_icache_all();
}

void flush_tlb(void) {
    /* __flush_tlb drops the I-cache before the TLB */
    flush_dcache();
    __flush_tlb();
}

/* No per-line publish on this platform: fall back to the full flush_tlb(). */
void flush_tlb_nosweep(void) {
    flush_tlb();
}

void dcache_clean_code_range(const void* start, uint32_t size) {
    if(size == 0)
        return;
    __dcache_clean_range((ewokos_addr_t)start, (ewokos_addr_t)start + size);
}

void dcache_flush_range(const void* start, uint32_t size) {
    if(size == 0)
        return;
    __dcache_flush_range((ewokos_addr_t)start, (ewokos_addr_t)start + size);
}

void flush_tlb_addr(ewokos_addr_t addr) {
    /* ARMv5/v6 lack the ARMv7 table-line publish path, so keep the original
     * whole D-cache clean plus TLB invalidate; a bare TLBI would leave dirty
     * page tables unpublished. */
    (void)addr;
    flush_tlb();
}

void set_translation_table_base(ewokos_addr_t tlb_base) {
    /*
     * cpu_arm926_switch_mm order: write back and drop the outgoing space's
     * lines while its translations are still live, then move the table
     * pointer, then invalidate I-cache and TLB.
     */
    flush_dcache();
    __set_translation_table_base(tlb_base);
    __flush_tlb();
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
