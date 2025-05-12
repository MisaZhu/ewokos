#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* p, uint32_t pte_attr) {
	(void)pte_attr;
	page_table_entry_v5_t *pte = (page_table_entry_v5_t*)p;
	pte->writeback = 0;
	pte->cacheable = 0;

	pte->ap3 = pte->ap2 = pte->ap1 = pte->ap0;
	
	if(pte_attr == PTE_ATTR_WRBACK) { //normal mem, write back
		pte->cacheable = 1;
		pte->writeback = 1;
	}
	else if(pte_attr == PTE_ATTR_WRBACK_ALLOCATE) { //write back allocate mem
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
	else if(pte_attr == PTE_ATTR_NOCACHE) { //nocache mem
		pte->cacheable = 0;
		pte->writeback = 1;
	}
	else if(pte_attr == PTE_ATTR_STRONG_ORDER) { //strong ordered mem
		pte->cacheable = 0;
		pte->writeback = 0;
	}
}

