#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <kstring.h>
#include <stddef.h>
#include <kernel/proc.h>
#include <kernel/system.h>
#include <kernel/kernel.h>

/*
 * map_pages adds the given virtual to physical memory mapping to the given
 * virtual memory. A mapping can map multiple pages.
 */
//void __attribute__((optimize("O0"))) map_pages(page_dir_entry_t *vm, uint32_t vaddr, uint32_t pstart, uint32_t pend, uint32_t permissions, uint32_t is_dev) {
void map_pages(page_dir_entry_t *vm, uint32_t vaddr, uint32_t pstart, uint32_t pend, uint32_t permissions, uint32_t is_dev) {
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
		map_page(vm,  virtual_current, physical_current, permissions, is_dev);
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

inline void vm_flush_tlb(page_dir_entry_t* vm) {
	page_dir_entry_t* old_vm = _current_proc == NULL ? _kernel_vm : _current_proc->space->vm;
	if(old_vm == vm) {
		flush_tlb();
		return;
	}

	set_translation_table_base((uint32_t)V2P(vm));
	set_translation_table_base((uint32_t)V2P(old_vm));
}
