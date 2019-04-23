#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <types.h>
#include <kstring.h>
#include <proc.h>
#include <system.h>

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

	uint32_t page_dir_index = PAGE_DIR_INDEX(virtualAddr);
	uint32_t page_index = PAGE_INDEX(virtualAddr);

	/* if this page_dirEntry is not mapped before, map it to a new page table */
	if (vm[page_dir_index].type == 0) {
		page_table = kalloc1k();
		memset(page_table, 0, PAGE_TABLE_SIZE);
		
		vm[page_dir_index].base = PAGE_TABLE_TO_BASE(V2P(page_table));
		vm[page_dir_index].type = PAGE_DIR_TYPE;
		vm[page_dir_index].domain = 0;
	}
	/* otherwise use the previously allocated page table */
	else {
		page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[page_dir_index].base));
	}

	/* map the virtual page to physical page in page table */
	page_table[page_index].type = PAGE_TYPE,
	page_table[page_index].bufferable = 0;
	page_table[page_index].cacheable = 0;
	page_table[page_index].permissions = permissions;
	page_table[page_index].base = PAGE_TO_BASE(physical);
}

/* unmap_page clears the mapping for the given virtual address */
void unmap_page(page_dir_entry_t *vm, uint32_t virtualAddr) {
	page_table_entry_t *page_table = NULL;
	uint32_t page_dir_index = PAGE_DIR_INDEX(virtualAddr);
	uint32_t page_index = PAGE_INDEX(virtualAddr);
	page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[page_dir_index].base));
	page_table[page_index].type = 0;
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
	uint32_t base_address = 0;

	pdir = (page_dir_entry_t*)((uint32_t) vm | ((virtual >> 20) << 2));
	base_address = pdir->base << 10;
	page = (page_table_entry_t*)((uint32_t) base_address | ((virtual >> 10) & 0x3fc));
	page = (page_table_entry_t*)P2V(page);
	result = (page->base << 12) | (virtual & 0xfff);
	return result;
}

/*
get page entry(virtual addr) by virtual address
*/
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, uint32_t virtual) {
	page_dir_entry_t *pdir = NULL;
	page_table_entry_t *page = NULL;
	uint32_t base_address = 0;

	pdir = (void *) ((uint32_t) vm | ((virtual >> 20) << 2));
	base_address = pdir->base << 10;
	page = (page_table_entry_t*)((uint32_t) base_address | ((virtual >> 10) & 0x3fc));
	page = (page_table_entry_t*)P2V(page);
	return page;
}

void free_page_tables(page_dir_entry_t *vm) {
	for (int i = 0; i < PAGE_DIR_NUM; i++) {
		if (vm[i].type != 0) {
			void *page_table = (void *) P2V(BASE_TO_PAGE_TABLE(vm[i].base));
			kfree1k(page_table);
		}
	}
}

static int32_t _p_mmu_lock = 0;

void _abort_entry(uint32_t v_addr) {
	CRIT_IN(_p_mmu_lock)	
	_current_proc->context[LR] = _current_proc->context[R2];
	v_addr = ALIGN_DOWN(v_addr, PAGE_SIZE);
	if(v_addr != 0 && v_addr <= _current_proc->space->heap_size) { //page fault, copy on write 
		CRIT_IN(_p_mmu_lock)	
		char *page = kalloc();
		if(page != NULL) {
			map_page(_current_proc->space->vm, 
					v_addr,
					V2P(page),
					AP_RW_RW);
			memcpy(page, (void*)v_addr, PAGE_SIZE);
			CRIT_OUT(_p_mmu_lock)	
			return;
		}
	}
	CRIT_OUT(_p_mmu_lock)
	proc_exit(_current_proc); //abort for wrong address!
}
