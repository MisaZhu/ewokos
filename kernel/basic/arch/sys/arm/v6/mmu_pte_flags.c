#include <mm/mmu.h>

//inline void __attribute__((optimize("O0"))) set_extra_flags(page_table_entry_t* pte, uint32_t is_dev) {
inline void set_pte_flags(page_table_entry_t* pte, uint32_t is_dev) {
	(void)is_dev;
	pte->bufferable = 0;
	pte->cacheable = 0;
	if(pte->ap == AP_RW_RW) {
		pte->tex = 0x7;
		pte->apx = 1;
		pte->ng = 1;
	}
	else {
		pte->tex = 0x5;
		pte->apx = 0;
	}
	pte->sharable = 1;
}

