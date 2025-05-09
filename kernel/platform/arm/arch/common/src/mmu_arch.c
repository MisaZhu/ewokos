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
int32_t map_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr,
		     ewokos_addr_t physical, uint32_t permissions, uint32_t pte_attr) {
	page_table_entry_t *page_table = 0;

	ewokos_addr_t page_dir_index = PAGE_DIR_INDEX(virtual_addr);
	ewokos_addr_t page_index = PAGE_INDEX(virtual_addr);

	/* if this page_dirEntry is not mapped before, map it to a new page table */
	if (vm[page_dir_index].type == 0) {
		page_table = kalloc1k();
		if(page_table == NULL)
			return -1;

		memset(page_table, 0, PAGE_TABLE_SIZE);
		vm[page_dir_index].type = PAGE_DIR_2LEVEL_TYPE;
		vm[page_dir_index].sbz= 0;
		vm[page_dir_index].domain = 0;
		vm[page_dir_index].base = PAGE_TABLE_TO_BASE(V2P(page_table));
	}
	/* otherwise use the previously allocated page table */
	else {
		page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[page_dir_index].base));
	}

	/* map the virtual page to physical page in page table */
	page_table[page_index].type = SMALL_PAGE_TYPE,
	page_table[page_index].base = PAGE_TO_BASE(physical);
	page_table[page_index].ap = permissions;
	set_pte_flags(&page_table[page_index], pte_attr);
	return 0;
}

/* unmap_page clears the mapping for the given virtual address */
void unmap_page(page_dir_entry_t *vm, ewokos_addr_t virtual_addr) {
	page_table_entry_t *page_table = 0;
	ewokos_addr_t page_dir_index = PAGE_DIR_INDEX(virtual_addr);
	ewokos_addr_t page_index = PAGE_INDEX(virtual_addr);
	page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[page_dir_index].base));
	page_table[page_index].type = 0;
}

/*
 * resolve_physical_address simulates the virtual memory hardware and maps the
 * given virtual address to physical address. This function can be used for
 * debugging if given virtual memory is constructed correctly.
 */
ewokos_addr_t resolve_phy_address(page_dir_entry_t *vm, ewokos_addr_t virtual) {
	page_dir_entry_t *pdir = 0;
	page_table_entry_t *page = 0;
	ewokos_addr_t result = 0;
	ewokos_addr_t base_address = 0;

	pdir = (page_dir_entry_t*)((ewokos_addr_t) vm | ((virtual >> 20) << 2));
	base_address = pdir->base << 10;
	page = (page_table_entry_t*)((ewokos_addr_t) base_address | ((virtual >> 10) & 0x3fc));
	page = (page_table_entry_t*)P2V(page);
	result = (page->base << 12) | (virtual & 0xfff);
	return result;
}

/*
get page entry(virtual addr) by virtual address
*/
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, ewokos_addr_t virtual) {
	page_dir_entry_t *pdir = 0;
	page_table_entry_t *page = 0;
	ewokos_addr_t base_address = 0;

	pdir = (void *) ((ewokos_addr_t) vm | ((virtual >> 20) << 2));
	base_address = pdir->base << 10;
	page = (page_table_entry_t*)((ewokos_addr_t) base_address | ((virtual >> 10) & 0x3fc));
	page = (page_table_entry_t*)P2V(page);
	return page;
}


void free_page_tables(page_dir_entry_t *vm) {
	int i;
	for (i = 0; i < PAGE_DIR_NUM; i++) {
		if (vm[i].type != 0) {
			void *page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[i].base));
			if(page_table != NULL)
				kfree1k(page_table);
		}
	}
}