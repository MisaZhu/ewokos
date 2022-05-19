#ifndef MMU_H
#define MMU_H

#include <mm/mmu_arch.h>

void map_pages(page_dir_entry_t *vm, uint32_t vaddr, 
	uint32_t pstart, 
	uint32_t pend,  
	uint32_t access_permissions,
	uint32_t no_cache);

void map_pages_size(page_dir_entry_t *vm, uint32_t vaddr, 
	uint32_t pstart, 
	uint32_t size,  
	uint32_t access_permissions,
	uint32_t no_cache);

void unmap_pages(page_dir_entry_t *vm, uint32_t virtual_addr, uint32_t pages);

void map_page_ref(page_dir_entry_t *vm, uint32_t vaddr, uint32_t paddr, uint32_t permissions);
void unmap_page_ref(page_dir_entry_t *vm, uint32_t vaddr);

void free_page_tables(page_dir_entry_t *vm);
uint32_t resolve_kernel_address(page_dir_entry_t *vm, uint32_t virtual);

#endif
