#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* p, uint32_t pte_attr) {
    (void)pte_attr;
    page_table_entry_v5_t *pte = (page_table_entry_v5_t*)p;
    pte->writeback = 0;
    pte->cacheable = 0;

    pte->ap3 = pte->ap2 = pte->ap1 = pte->ap0;
    
    /*
     * ARM926EJ-S (ARMv5) D-cache runs Write-Through, No-Write-Allocate for all
     * normal memory (C=1, B=0), NOT Write-Back (C=1, B=1).
     *
     * Why: the D-cache is VIVT with no ASID, and when it is enabled the MMU's
     * translation-table walk is itself cached and reads PTEs by PHYSICAL address
     * (a different VIVT tag than the kernel P2V alias the CPU writes them
     * through). In Write-Back mode a modified PTE - or any data a DMA master /
     * cross-VA reader depends on - stays dirty in the cache and reaches DRAM only
     * when a set/way sweep happens to run, so the walker intermittently reads
     * stale DRAM. Hardware A/B proved this exactly: I-cache-only and no-cache
     * boot reliably, but enabling the D-cache in Write-Back is intermittent.
     *
     * Write-Through makes every store land in DRAM immediately, so the walker
     * and all other observers are always coherent, while read-caching (the
     * dominant speedup) is fully retained. The other VIVT hazard - stale CLEAN
     * lines aliasing across an address-space switch - is already covered by the
     * full c7,c6,0 invalidate in set_translation_table_base(). Write-Back's only
     * edge (lazy dirty eviction) is largely wasted here anyway, since the whole
     * D-cache is invalidated on every switch_mm.
     */
    if(pte_attr == PTE_ATTR_WRBACK) { //normal mem -> write-through (see above)
        pte->cacheable = 1;
        pte->writeback = 0;
    }
    else if(pte_attr == PTE_ATTR_WRBACK_ALLOCATE) { //write back allocate -> write-through
        pte->cacheable = 1;
        pte->writeback = 0;
    }
    else if(pte_attr == PTE_ATTR_WRTHR) { //write throuh mem
        pte->cacheable = 1;
        pte->writeback = 0;
    }
    else if(pte_attr == PTE_ATTR_DEV) { //dev mem
        pte->cacheable = 0;
        pte->writeback = 1;
    }
    else if(pte_attr == PTE_ATTR_NOCACHE) { //nocache mem
        pte->cacheable = 0;
        pte->writeback = 1;
    }
    else if(pte_attr == PTE_ATTR_STRONG_ORDER) { //strong ordered mem
        pte->cacheable = 0;
        pte->writeback = 0;
    }
}

