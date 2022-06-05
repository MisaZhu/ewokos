#ifndef MMU_ARCH_H
#define MMU_ARCH_H

#include "mmudef.h"

#define PAGE_DIR_INDEX(x) ((uint32_t)x >> 20)
#define PAGE_INDEX(x) (((uint32_t)x >> 12) & 255)

#define PAGE_TABLE_TO_BASE(x) ((uint32_t)x >> 10)
#define BASE_TO_PAGE_TABLE(x) ((void *) ((uint32_t)x << 10))
#define PAGE_TO_BASE(x) ((uint32_t)x >> 12)

/* a 32-bit entry in hardware's PageDir table */
typedef struct {
	uint32_t type   : 2; // 0: fault, 0x1: page table, 0x2: section
	uint32_t sbz    : 3; //should be zero
	uint32_t domain : 4;
	uint32_t p      : 1; //ECC Enable, ignored by ARM1176JZF-S
	uint32_t base   : 22;
} page_dir_entry_t;

/* a 32-bit entry in hardware's page table */
typedef struct {
	uint32_t type       : 2; //0: fault, 0x1: large size(64k), 0x2: small size(4k), 0x3: small size with no-excute(4k)
	uint32_t bufferable : 1; //B
	uint32_t cacheable  : 1; //C
	uint32_t ap         : 2; //Access Permissions,
                           //0x1: super-RW,user-NONE
                           //0x2 super-RW,user-R
                           //0x3 super-RW,user-RW
	uint32_t tex        : 3; //Type Extension Field, access control work with C,B
	uint32_t apx        : 1; //Access Permissions Extension Bit
	uint32_t sharable   : 1;
	uint32_t ng         : 1; //Not-Global, 0 for global, 1 for local
	uint32_t base       : 20;
} page_table_entry_t; 

void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr);

#define PTE_ATTR_WRBACK      0
#define PTE_ATTR_DEV         1
#define PTE_ATTR_WRTHR       2
#define PTE_ATTR_KIMG        3

int32_t  map_page(page_dir_entry_t *vm, 
  uint32_t virtual_addr, 
	uint32_t physical,
	uint32_t access_permissions, 
	uint32_t pte_attr);

void unmap_page(page_dir_entry_t *vm, uint32_t virtual_addr);

uint32_t resolve_phy_address(page_dir_entry_t *vm, uint32_t virtual);
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, uint32_t virtual);

#endif
