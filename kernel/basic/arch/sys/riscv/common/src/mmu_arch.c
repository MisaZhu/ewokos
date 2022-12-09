#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <stddef.h>
#include <kernel/system.h>
#include <kernel/kernel.h>

/*
 * map_page adds to the given virtual memory the mapping of a single virtual page
 * to a physical page.
 * Notice: virtual and physical address inputed must be all aliend by PAGE_SIZE !
 */
int32_t map_page(page_dir_entry_t *vm, uint32_t virtual_addr,
		     uint32_t physical, uint32_t permissions, uint32_t pte_attr) {
	page_table_entry_t *level1 = 0;
	page_table_entry_t *level0 = 0;
	uint32_t ppn2 = PAGE_DIR_PPN2(virtual_addr);
	uint32_t ppn1 = PAGE_DIR_PPN1(virtual_addr);
	uint32_t ppn0 = PAGE_DIR_PPN0(virtual_addr);
	/* if this page_dirEntry is not mapped before, map it to a new page table */
	if (vm[ppn2].valid == 0) {
		level1 = kalloc4k();
		if(level1 == NULL)
			return -1;

		memset(level1, 0, PAGE_TABLE_SIZE);
		vm[ppn2].valid = 1;
		vm[ppn2].permission = 0;
		vm[ppn2].ppn = PAGE_TABLE_TO_BASE(V2P(level1)); 
	}else{
		level1 = (page_table_entry_t *)P2V(BASE_TO_PAGE_TABLE(vm[ppn2].ppn));
	}

	/* if this page_dirEntry is not mapped before, map it to a new page table */
	if (level1[ppn1].valid == 0) {
		level0 = kalloc4k();
		if(level0 == NULL)
			return -1;

		memset(level0, 0, PAGE_TABLE_SIZE);
		level1[ppn1].valid = 1;
		level1[ppn1].permission = 0;
		level1[ppn1].ppn = PAGE_TABLE_TO_BASE(V2P(level0)); 
	}else{
		level0 = (page_table_entry_t *)P2V(BASE_TO_PAGE_TABLE(level1[ppn1].ppn));
	}

	/* map the virtual page to physical page in page table */
	level0[ppn0].valid = 1;
	level0[ppn0].access = 1;
	level0[ppn0].dirty = 1;
	level0[ppn0].ppn = PAGE_TABLE_TO_BASE(physical); 
	set_pte_permission(&level0[ppn0], permissions);
	set_pte_flags(&level0[ppn0], pte_attr);
	return 0;
}

/* unmap_page clears the mapping for the given virtual address */
void unmap_page(page_dir_entry_t *vm, uint32_t virtual_addr) {
	page_table_entry_t *level1 = 0;
	page_table_entry_t *level0 = 0;

	uint32_t ppn2 = PAGE_DIR_PPN2(virtual_addr);
	uint32_t ppn1 = PAGE_DIR_PPN1(virtual_addr);
	uint32_t ppn0 = PAGE_DIR_PPN0(virtual_addr);

	if(vm[ppn2].valid){
		level1 = P2V(BASE_TO_PAGE_TABLE(vm[ppn2].ppn));
		if(level1[ppn1].valid){
			level0 = P2V(BASE_TO_PAGE_TABLE(level1[ppn1].ppn));
			if(level0[ppn0].valid){
				level0[ppn0].valid = 0;
			}
		}
	}

	/*TODO:
	 * 	need release empyt level0 page table for save memory
	 */	
}

/*
 * resolve_physical_address simulates the virtual memory hardware and maps the
 * given virtual address to physical address. This function can be used for
 * debugging if given virtual memory is constructed correctly.
 */
uint32_t resolve_phy_address(page_dir_entry_t *vm, uint32_t virtual) {
	page_table_entry_t *level1 = 0;
	page_table_entry_t *level0 = 0;

	uint32_t ppn2 = PAGE_DIR_PPN2(virtual);
	uint32_t ppn1 = PAGE_DIR_PPN1(virtual);
	uint32_t ppn0 = PAGE_DIR_PPN0(virtual);

	if(vm[ppn2].valid){
		level1 = P2V(BASE_TO_PAGE_TABLE(vm[ppn2].ppn));
		if(level1[ppn1].valid){
			level0 = P2V(BASE_TO_PAGE_TABLE(level1[ppn1].ppn));
			if(level0[ppn0].valid){
				return BASE_TO_PAGE_TABLE(level0[ppn0].ppn) + (virtual&0xFFF);   
			}
		}
	}
}

/*
get page entry(virtual addr) by virtual address
*/
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, uint32_t virtual) {
	page_table_entry_t *level1 = 0;
	page_table_entry_t *level0 = 0;

	uint32_t ppn2 = PAGE_DIR_PPN2(virtual);
	uint32_t ppn1 = PAGE_DIR_PPN1(virtual);

	if(vm[ppn2].valid){
		level1 = P2V(BASE_TO_PAGE_TABLE(vm[ppn2].ppn));
		if(level1[ppn1].valid){
			return P2V(BASE_TO_PAGE_TABLE(level1[ppn1].ppn));
		}
	}
	return NULL;
}


void free_page_tables(page_dir_entry_t *vm) {
	for(int ppn2 = 0 ; ppn2 < PAGE_DIR_NUM; ppn2++){
		if(vm[ppn2].valid){
			page_table_entry_t *level1 = P2V(BASE_TO_PAGE_TABLE(vm[ppn2].ppn));
			for(int ppn1 = 0; ppn1 < PAGE_DIR_NUM; ppn1++){
				if(level1[ppn1].valid){
					page_table_entry_t *level0 = P2V(BASE_TO_PAGE_TABLE(level1[ppn1].ppn));
					kfree4k(level0);
				}
			}
			kfree4k(level1);
		}
	}
}