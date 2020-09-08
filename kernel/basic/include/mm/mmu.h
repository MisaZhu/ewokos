#ifndef MMU_H
#define MMU_H

#include <mm/mmu_arch.h>

void map_pages(page_dir_entry_t *vm, uint32_t vaddr, 
	uint32_t pstart, 
	uint32_t pend,  
	uint32_t access_permissions,
	uint32_t is_dev);

void unmap_pages(page_dir_entry_t *vm, uint32_t virtual_addr, uint32_t pages);

void free_page_tables(page_dir_entry_t *vm);
uint32_t resolve_kernel_address(page_dir_entry_t *vm, uint32_t virtual);
void vm_flush_tlb(page_dir_entry_t* vm);

extern uint32_t _mmio_base;

#endif
