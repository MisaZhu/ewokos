#include <mm/mmu.h>
#include <mm/kmalloc.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <stddef.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <kernel/kernel.h>

ewokos_addr_t get_allocable_start(void) {
	return KMALLOC_END;
}

/*
 * map_pages adds the given virtual to physical memory mapping to the given
 * virtual memory. A mapping can map multiple pages.
 */
void map_pages(page_dir_entry_t *vm, ewokos_addr_t vaddr, ewokos_addr_t pstart, ewokos_addr_t pend, uint32_t permissions, uint32_t pte_attr) {
	ewokos_addr_t physical_current = 0;
	ewokos_addr_t virtual_current = 0;

	ewokos_addr_t virtual_start = ALIGN_DOWN(vaddr, PAGE_SIZE);
	ewokos_addr_t physical_start = ALIGN_DOWN(pstart, PAGE_SIZE);
	ewokos_addr_t physical_end = ALIGN_UP( pend, PAGE_SIZE);

	/* iterate over pages and map each page */
	virtual_current = virtual_start;
	for (physical_current = physical_start; 
			physical_current < physical_end;
			physical_current += PAGE_SIZE) {
		map_page(vm,  virtual_current, physical_current, permissions, pte_attr);
		virtual_current += PAGE_SIZE;
	}
}

void map_pages_size(page_dir_entry_t *vm, ewokos_addr_t vaddr, ewokos_addr_t pstart, uint32_t size, uint32_t permissions, uint32_t pte_attr) {
	map_pages(vm, vaddr, pstart, pstart + size, permissions, pte_attr);
}

void unmap_pages(page_dir_entry_t *vm, ewokos_addr_t virtual_addr, uint32_t pages) {
	uint32_t i;
	for(i=0; i<pages; i++) {
		unmap_page(vm, virtual_addr + PAGE_SIZE*i);
	}
}

inline ewokos_addr_t resolve_kernel_address(page_dir_entry_t *vm, ewokos_addr_t virtual) {
	ewokos_addr_t phy = resolve_phy_address(vm, virtual);
	return P2V(phy);
}

inline void map_page_ref(page_dir_entry_t *vm, ewokos_addr_t vaddr, ewokos_addr_t paddr, uint32_t permissions, uint32_t pte_attr) {
	map_page(vm, vaddr, paddr, permissions, pte_attr);
	uint32_t i = page_ref_index(paddr);
	if(i < _pages_ref.max)
		_pages_ref.refs[i]++;
}

inline void unmap_page_ref(page_dir_entry_t *vm, ewokos_addr_t virtual_addr) {
	ewokos_addr_t paddr = resolve_phy_address(vm, virtual_addr);
	uint32_t i = page_ref_index(paddr);
	if(i < _pages_ref.max) {
		if(_pages_ref.refs[i] > 0)
			_pages_ref.refs[i]--;
		if(_pages_ref.refs[i] == 0)
			kfree4k((void*)P2V(paddr));
	}
	unmap_page(vm, virtual_addr);
}
