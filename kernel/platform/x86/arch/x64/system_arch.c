/*
 * x86_64 cache / TLB / address-space maintenance.
 *
 * Platform-specific implementation of the interfaces declared in
 * <kernel/system.h>. Reproduces the `__x86_64__` branch that used to live in
 * the common kernel/kernel/src/system.c. x86 has hardware-coherent WB memory
 * (MESI + snoop DMA), so the range-based clean/flush helpers are no-ops and
 * there is no ASID tagging: single-page invalidation uses INVLPG and the
 * switch path is a plain CR3 reload - a full TLB invalidation on its own,
 * with no WBINVD / whole-cache sweep needed on coherent WB memory.
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
#endif
}

void flush_tlb(void) {
    /* Coherent WB memory + CR3 reload (a full non-global TLB invalidation)
     * needs no WBINVD / whole-cache sweep, SMP or UP alike. */
    __flush_tlb();
}

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
    __asm__ volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

void set_translation_table_base(ewokos_addr_t tlb_base) {
    /* Loading CR3 already invalidates non-global TLB entries; an extra
     * flush_tlb() would only repeat the reload. */
    __set_translation_table_base(tlb_base);
}

void set_translation_table_base_asid(ewokos_addr_t tlb_base, uint32_t asid) {
    (void)asid;
    set_translation_table_base(tlb_base);
}

void flush_tlb_asid(uint32_t asid) {
    (void)asid;
}

void wfi(void) {
    __asm__ volatile("hlt");
}
