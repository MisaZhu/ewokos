#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <types.h>
#include <kstring.h>
#include <printk.h>

/*
 * map_pages adds the given virtual to physical memory mapping to the given
 * virtual memory. A mapping can map multiple pages.
 */
void map_pages(page_dir_entry_t *vm, uint32_t vaddr, uint32_t pstart, uint32_t pend, int permissions) {
	uint32_t physical_current = 0;
	uint32_t virtual_current = 0;

	uint32_t virtual_start = ALIGN_DOWN(vaddr, PAGE_SIZE);
	uint32_t physical_start = ALIGN_DOWN(pstart, PAGE_SIZE);
	uint32_t physical_end = ALIGN_UP(pend, PAGE_SIZE);

	/* iterate over pages and map each page */
	virtual_current = virtual_start;
	for (physical_current = physical_start; 
			physical_current < physical_end;
			physical_current += PAGE_SIZE) {
		map_page(vm,  virtual_current, physical_current, permissions);
		virtual_current += PAGE_SIZE;
	}
}

/*
 * map_page adds to the given virtual memory the mapping of a single virtual page
 * to a physical page.
 * Notice: virtual and physical address inputed must be all aliend by PAGE_SIZE !
 */
void map_page(page_dir_entry_t *vm, uint32_t virtualAddr,
		     uint32_t physical, int permissions) {
	page_table_entry_t *page_table = NULL;

	uint32_t pageDirIndex = PAGE_DIR_INDEX(virtualAddr);
	uint32_t pageIndex = PAGE_INDEX(virtualAddr);

	/* if this pageDirEntry is not mapped before, map it to a new page table */
	if (vm[pageDirIndex].type == 0) {
		page_table = kalloc1k();
		memset(page_table, 0, PAGE_TABLE_SIZE);
		
		vm[pageDirIndex].base = PAGE_TABLE_TO_BASE(V2P(page_table));
		vm[pageDirIndex].type = PAGE_DIR_TYPE;
		vm[pageDirIndex].domain = 0;
	}
	/* otherwise use the previously allocated page table */
	else {
		page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[pageDirIndex].base));
	}

	/* map the virtual page to physical page in page table */
	page_table[pageIndex].type = PAGE_TYPE,
	page_table[pageIndex].bufferable = 0;
	page_table[pageIndex].cacheable = 0;
	page_table[pageIndex].permissions = permissions;
	page_table[pageIndex].base = PAGE_TO_BASE(physical);
}

/* unmap_page clears the mapping for the given virtual address */
void unmap_page(page_dir_entry_t *vm, uint32_t virtualAddr) {
	page_table_entry_t *page_table = NULL;
	uint32_t pageDirIndex = PAGE_DIR_INDEX(virtualAddr);
	uint32_t pageIndex = PAGE_INDEX(virtualAddr);
	page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[pageDirIndex].base));
	page_table[pageIndex].type = 0;
}

/*
 * resolve_physical_address simulates the virtual memory hardware and maps the
 * given virtual address to physical address. This function can be used for
 * debugging if given virtual memory is constructed correctly.
 */
uint32_t resolve_phy_address(page_dir_entry_t *vm, uint32_t virtual) {
	page_dir_entry_t *pdir = NULL;
	page_table_entry_t *page = NULL;
	uint32_t result = 0;
	void *baseAddress = 0;

	pdir = (void *) ((uint32_t) vm | ((virtual >> 20) << 2));
	baseAddress = (void *) (pdir->base << 10);
	page = (void *) ((uint32_t) baseAddress | ((virtual >> 10) & 0x3fc));
	page = (void *) P2V(page);
	result = (page->base << 12) | (virtual & 0xfff);

	return result;
}

void free_page_tables(page_dir_entry_t *vm) {
	for (int i = 0; i < PAGE_DIR_NUM; i++) {
		if (vm[i].type != 0) {
			void *page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[i].base));
			kfree1k(page_table);
		}
	}
}

