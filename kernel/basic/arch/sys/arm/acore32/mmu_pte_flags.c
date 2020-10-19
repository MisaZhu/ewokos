#include <mm/mmu.h>

//inline void __attribute__((optimize("O0"))) set_pte_flags(page_table_entry_t* pte, uint32_t is_dev) {
inline void set_pte_flags(page_table_entry_t* pte, uint32_t is_dev) {
	pte->bufferable = 0;
	pte->cacheable = 0;

	if(is_dev == 0) { //normal mem
		//pte->tex = 0x1;
		pte->cacheable = 1;
		pte->bufferable = 1;
	}
	else { //device 
		//pte->cacheable = 1;
		//pte->tex = 0x7;
	}
}

