#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* pte, uint64_t pte_attr) {
	pte->NSTable = 1;
	pte->EntryType = TYPE_PAGE;
	pte->AF = 1;
	pte->SH = STAGE2_SH_INNER_SHAREABLE;
	if(pte_attr == PTE_ATTR_DEV) {
		pte->PXN = 1;
		pte->UXN = 1;
		pte->MemAttr = MT_DEVICE_NGNRE;
	}
	else if(pte_attr == PTE_ATTR_NOCACHE)
		pte->MemAttr = MT_NORMAL_NC;
	else
		pte->MemAttr = MT_NORMAL;
}

