#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr) {
	pte->permission = 7;
	pte->access = 1;
	pte->dirty = 1;
	pte->global = 1;
#ifdef C906_EXTEND
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
		pte->strongorder = 1;
	}
	else if(pte_attr == PTE_ATTR_STRONG_ORDER) { //strong ordered mem
		pte->cacheable = 0;
		pte->writeback = 0;
		pte->strongorder = 1;
	}
#endif
}

inline void set_pte_permission(page_table_entry_t* pte, uint32_t permission) {
    switch(permission){
		case AP_RW_D:
		    pte->user = 0;
		    pte->permission = 7;
		    break;
		case AP_RW_R:
		    pte->user = 1;
		    pte->permission = 5;
			break;
		case AP_RW_RW:
			pte->user = 1;
			pte->permission = 7;
			break;
		break;
		default:
		    break;
	}
}

