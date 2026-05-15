#include <mm/mmu_arch.h>

void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr) {
	pte->pwt = 0;
	pte->pcd = 0;
	pte->pat = 0;
	pte->nx = 0;
	pte->global = 1;

	switch (pte_attr) {
	case PTE_ATTR_WRTHR:
		pte->pwt = 1;
		break;
	case PTE_ATTR_WRCOMB:
		pte->pwt = 1;
		pte->pat = 1;
		pte->global = 0;
		break;
	case PTE_ATTR_DEV:
	case PTE_ATTR_STRONG_ORDER:
	case PTE_ATTR_NOCACHE:
		pte->pcd = 1;
		pte->global = 0;
		break;
	case PTE_ATTR_WRBACK:
	case PTE_ATTR_WRBACK_ALLOCATE:
	default:
		break;
	}
}
