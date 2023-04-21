#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr) {
	pte->bufferable = 0;
	pte->cacheable = 0;
	pte->sharable = 1;

	if(pte_attr == PTE_ATTR_WRBACK) { //normal mem, write back
		pte->cacheable = 1;
		pte->bufferable = 1;
	}
	else if(pte_attr == PTE_ATTR_WRBACK_ALLOCATE) { //write back allocate mem
		pte->tex = 0x1;
		pte->cacheable = 1;
		pte->bufferable = 1;
	}
	else if(pte_attr == PTE_ATTR_WRTHR) { //write throuh mem
		pte->cacheable = 1;
		pte->bufferable = 0;
	}
	else if(pte_attr == PTE_ATTR_DEV) { //dev mem
		pte->cacheable = 0;
		pte->bufferable = 1;
	}
	else if(pte_attr == PTE_ATTR_STRONG_ORDER) { //strong ordered mem
		pte->cacheable = 0;
		pte->bufferable = 0;
	}
}

