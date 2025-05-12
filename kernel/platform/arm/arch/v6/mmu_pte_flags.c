#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr) {
	(void)pte_attr;
	pte->writeback = 0;
	pte->cacheable = 0;
	pte->apx = 1;
	pte->ng = 1;

	if(pte->ap == AP_RW_RW) {
		pte->tex = 0x7;
		pte->sharable = 1;
	}
	else {
		pte->tex = 0x2;
	}

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
	else if(pte_attr == PTE_ATTR_NOCACHE) { //nocache mem
		pte->cacheable = 0;
		pte->writeback = 1;
	}
	else if(pte_attr == PTE_ATTR_STRONG_ORDER) { //strong ordered mem
		pte->cacheable = 0;
		pte->writeback = 0;
	}
}

