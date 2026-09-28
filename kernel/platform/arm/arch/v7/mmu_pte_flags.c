#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr) {
    pte->writeback = 0;
    pte->cacheable = 0;
    pte->tex = 0;
    pte->sharable = 1;

    if(pte_attr == PTE_ATTR_WRBACK) { //normal mem, write back
        pte->cacheable = 1;
        pte->writeback = 1;
    }
    else if(pte_attr == PTE_ATTR_WRBACK_ALLOCATE) { //write back allocate mem
        pte->tex = 0x1;
        pte->cacheable = 1;
        pte->writeback = 1;
    }
    else if(pte_attr == PTE_ATTR_WRTHR) { //write throuh mem
        pte->cacheable = 1;
        pte->writeback = 0;
    }
    else if(pte_attr == PTE_ATTR_DEV) { //dev mem
        pte->cacheable = 0;
        pte->writeback = 1;
    }
    else if(pte_attr == PTE_ATTR_NOCACHE) {
        /* TRE=0：TEX=001、C=B=0 表示 Normal Non-cacheable。
         * TEX=000、B=1 是 Device，不能用于普通 RAM 的非对齐访问。 */
        pte->tex = 1;
    }
    else if(pte_attr == PTE_ATTR_STRONG_ORDER) { //strong ordered mem
        pte->cacheable = 0;
        pte->writeback = 0;
    }
}

