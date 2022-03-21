#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <stddef.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <kernel/kernel.h>

/*
 * map_pages adds the given virtual to physical memory mapping to the given
 * virtual memory. A mapping can map multiple pages.
 */
//void __attribute__((optimize("O0"))) map_pages(page_dir_entry_t *vm, uint32_t vaddr, uint32_t pstart, uint32_t pend, uint32_t permissions, uint32_t no_cache) {
void map_pages(page_dir_entry_t *vm, uint32_t vaddr, uint32_t pstart, uint32_t pend, uint32_t permissions, uint32_t no_cache) {
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
		map_page(vm,  virtual_current, physical_current, permissions, no_cache);
		virtual_current += PAGE_SIZE;
	}
}

void unmap_pages(page_dir_entry_t *vm, uint32_t virtual_addr, uint32_t pages) {
	uint32_t i;
	for(i=0; i<pages; i++) {
		unmap_page(vm, virtual_addr + PAGE_SIZE*i);
	}
}

inline uint32_t resolve_kernel_address(page_dir_entry_t *vm, uint32_t virtual) {
	uint32_t phy = resolve_phy_address(vm, virtual);
	return P2V(phy);
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

inline void map_page_ref(page_dir_entry_t *vm, uint32_t vaddr, uint32_t paddr, uint32_t permissions) {
	map_page(vm, vaddr, paddr, permissions, 0);
	uint32_t i = page_ref_index(paddr);
	if(i < _pages_ref.max)
		_pages_ref.refs[i]++;
}

inline void unmap_page_ref(page_dir_entry_t *vm, uint32_t virtual_addr) {
	uint32_t paddr = resolve_phy_address(vm, virtual_addr);
	unmap_page(vm, virtual_addr);

	uint32_t i = page_ref_index(paddr);
	if(i < _pages_ref.max) {
		_pages_ref.refs[i]--;
		if(_pages_ref.refs[i] <= 0) {
			kfree4k((void*)P2V(paddr));
			_pages_ref.refs[i] = 0;
		}
	}
}

