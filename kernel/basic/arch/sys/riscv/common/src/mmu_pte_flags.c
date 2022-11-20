#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr) {
	pte->permission = 7;
			pte->global = 1;
	switch(pte_attr){
		case PTE_ATTR_DEV:
			pte->global = 1;
			break;
		default:
			break;
	}
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
		default:
		    break;
	}
}

