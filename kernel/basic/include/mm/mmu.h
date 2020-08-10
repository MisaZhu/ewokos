#ifndef MMU_H
#define MMU_H

#include <mm/mmudef.h>

#define PAGE_DIR_INDEX(x) ((uint32_t)x >> 20)
#define PAGE_INDEX(x) (((uint32_t)x >> 12) & 255)

#define PAGE_TABLE_TO_BASE(x) ((uint32_t)x >> 10)
#define BASE_TO_PAGE_TABLE(x) ((void *) ((uint32_t)x << 10))
#define PAGE_TO_BASE(x) ((uint32_t)x >> 12)

/* a 32-bit entry in hardware's PageDir table */
typedef struct {
	uint32_t type : 2;
	uint32_t : 3;
	uint32_t domain : 4;
	uint32_t : 1;
	uint32_t base : 22;
} page_dir_entry_t;

/* a 32-bit entry in hardware's page table */
typedef struct {
	uint32_t type : 2;
	uint32_t bufferable : 1;
	uint32_t cacheable : 1;
	uint32_t permissions : 8;
	uint32_t base : 20;
} page_table_entry_t; 

void map_pages(page_dir_entry_t *vm, uint32_t vaddr, 
	uint32_t pstart, 
	uint32_t pend,  
	uint32_t access_permissions);

int32_t  map_page(page_dir_entry_t *vm, 
  uint32_t virtual_addr, 
	uint32_t physical,
	uint32_t access_permissions);

void unmap_page(page_dir_entry_t *vm, uint32_t virtual_addr);
void unmap_pages(page_dir_entry_t *vm, uint32_t virtual_addr, uint32_t pages);

void free_page_tables(page_dir_entry_t *vm);
uint32_t resolve_phy_address(page_dir_entry_t *vm, uint32_t virtual);
uint32_t resolve_kernel_address(page_dir_entry_t *vm, uint32_t virtual);
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, uint32_t virtual);

extern uint32_t _mmio_base;

#endif
