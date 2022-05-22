#include <mm/mmu.h>

inline void set_pte_flags(page_table_entry_t* pte, uint32_t no_cache) {
	pte->bufferable = 0;
	pte->cacheable = 0;
	pte->sharable = 1;

	if(no_cache == 0) { //normal mem
		//pte->tex = 0x1;
		pte->cacheable = 1;
		//pte->bufferable = 1;
	}
}

