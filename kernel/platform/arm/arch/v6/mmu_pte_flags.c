#include <mm/mmu.h>

/*
 * ARMv6 short-descriptor memory types (TRE = 0, so TEX/C/B select the type
 * directly). Only these combinations are defined; anything else is reserved
 * and behaves unpredictably:
 *   TEX=000 C=0 B=0  Strongly-ordered
 *   TEX=000 C=0 B=1  Shared Device
 *   TEX=001 C=0 B=0  Normal, Non-cacheable
 *   TEX=001 C=1 B=0  Normal, Write-Through, no write-allocate
 *   TEX=001 C=1 B=1  Normal, Write-Back, no write-allocate
 *   TEX=010 C=1 B=1  Normal, Write-Back, write-allocate
 * Note TEX=010/011/1xx with C=0 B=1 is "Normal, Write-Back" (cacheable), so
 * PTE_ATTR_DEV must not use TEX=001 - that would cache MMIO.
 */
inline void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr) {
    pte->writeback = 0;
    pte->cacheable = 0;
    pte->tex = 0;
    pte->sharable = 0;
    pte->apx = 1;
    /* v6 system_arch.c ignores ASIDs and does a full TLB flush on every
     * address-space switch, so every entry is kept global */
    pte->ng = 0;

    if(pte_attr == PTE_ATTR_WRBACK) { //normal mem, write back
        pte->tex = 1;
        pte->cacheable = 1;
        pte->writeback = 1;
    }
    else if(pte_attr == PTE_ATTR_WRBACK_ALLOCATE) { //write back allocate mem
        pte->tex = 2;
        pte->cacheable = 1;
        pte->writeback = 1;
    }
    else if(pte_attr == PTE_ATTR_WRTHR) { //write throuh mem
        pte->tex = 1;
        pte->cacheable = 1;
    }
    else if(pte_attr == PTE_ATTR_DEV) { //dev mem
        pte->writeback = 1;
    }
    else if(pte_attr == PTE_ATTR_NOCACHE) { //nocache mem
        pte->tex = 1;
    }
    /* PTE_ATTR_STRONG_ORDER keeps TEX=000 C=0 B=0 */
}

