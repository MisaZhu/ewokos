#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* pte, uint64_t pte_attr) {
    pte->NSTable = 1;
    pte->EntryType = TYPE_PAGE;
    pte->AF = 1;
    pte->SH = STAGE2_SH_INNER_SHAREABLE;
    /* map_page reuses a slot without zeroing it and unmap_page only clears
     * EntryType, so a slot that was once Device keeps PXN/UXN=1. Reset them
     * here: only Device memory is execute-never, ordinary RAM must stay
     * executable (the user-stack noexec hook re-sets UXN after map_page). */
    pte->PXN = 0;
    pte->UXN = 0;
    if(pte_attr == PTE_ATTR_DEV) {
        pte->PXN = 1;
        pte->UXN = 1;
        /* Device-nGnRNE, outer shareable: must match the kernel/boot
         * MMIO mappings; on BCM2712 a Device-nGnRE/inner-shareable
         * mapping of peripherals above 4GB gets a sync external abort
         * when accessed from EL0. */
        pte->SH = STAGE2_SH_OUTER_SHAREABLE;
        pte->MemAttr = MT_DEVICE_NGNRNE;
    }
    else if(pte_attr == PTE_ATTR_NOCACHE)
        pte->MemAttr = MT_NORMAL_NC;
    else
        pte->MemAttr = MT_NORMAL;
}

