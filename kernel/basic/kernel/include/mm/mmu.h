#ifndef MMU_H
#define MMU_H

#include <mm/mmu_arch.h>
#include <kernel/hw_info.h>

#define V2P(V) ((uint32_t)(V) - KERNEL_BASE + _sys_info.phy_offset)
#define P2V(P) ((uint32_t)(P) + KERNEL_BASE - _sys_info.phy_offset)

void map_pages(page_dir_entry_t *vm, uint32_t vaddr, 
	uint32_t pstart, 
	uint32_t pend,  
	uint32_t access_permissions,
	uint32_t pte_attr);

void map_pages_size(page_dir_entry_t *vm, uint32_t vaddr, 
	uint32_t pstart, 
	uint32_t size,  
	uint32_t access_permissions,
	uint32_t pte_attr);

void unmap_pages(page_dir_entry_t *vm, uint32_t virtual_addr, uint32_t pages);

void map_page_ref(page_dir_entry_t *vm, uint32_t vaddr, uint32_t paddr, uint32_t permissions);
void unmap_page_ref(page_dir_entry_t *vm, uint32_t vaddr);

void free_page_tables(page_dir_entry_t *vm);
uint32_t resolve_kernel_address(page_dir_entry_t *vm, uint32_t virtual);

#endif
