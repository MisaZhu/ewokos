#ifndef MMU_ARCH_H
#define MMU_ARCH_H

#include "mmudef.h"

#define PAGE_DIR_PPN2(x) (((uint32_t)x >> 30) & (0x1ff))
#define PAGE_DIR_PPN1(x) (((uint32_t)x >> 21) & (0x1ff))
#define PAGE_DIR_PPN0(x) (((uint32_t)x >> 12) & (0x1ff))

#define PAGE_INDEX(x) (((uint32_t)x >> 12) & 255)

#define PAGE_TABLE_TO_BASE(x) ((uint32_t)x >> 12)
#define BASE_TO_PAGE_TABLE(x) (((uint32_t)x << 12))
#define PAGE_TO_BASE(x) ((uint32_t)x >> 12)

#pragma pack(1)
/* a 64-bit entry in hardware's PageDir table */
typedef struct {
	uint8_t valid 			: 1;
	uint8_t permission 		: 3;
	uint8_t user 			: 1;
	uint8_t global 			: 1;
	uint8_t access 			: 1;
	uint8_t dirty 			: 1;
	uint8_t software      	: 2;
	uint64_t ppn			: 44;
    uint32_t reserved  		: 10;
} page_dir_entry_t;

/* a 64-bit entry in hardware's page table */
typedef struct {
	uint8_t valid 			: 1;
	uint8_t permission 		: 3;
	uint8_t user 			: 1;
	uint8_t global 			: 1;
	uint8_t access 			: 1;
	uint8_t dirty 			: 1;
	uint8_t software      	: 2;
	uint64_t ppn			: 44;
    uint32_t reserved  		: 7;
	/*c906 extend*/
	uint8_t  writeback         : 1;
	uint8_t  cacheable          : 1;
	uint8_t  strongorder		: 1;
} page_table_entry_t; 
#pragma pack()

void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr);
void set_pte_permission(page_table_entry_t* pte, uint32_t permission);

#define PTE_ATTR_WRBACK          0
#define PTE_ATTR_DEV             1
#define PTE_ATTR_WRTHR           2
#define PTE_ATTR_WRBACK_ALLOCATE 3
#define PTE_ATTR_STRONG_ORDER    4

int32_t  map_page(page_dir_entry_t *vm, 
  uint32_t virtual_addr, 
	uint32_t physical,
	uint32_t access_permissions, 
	uint32_t pte_attr);

void unmap_page(page_dir_entry_t *vm, uint32_t virtual_addr);

uint32_t resolve_phy_address(page_dir_entry_t *vm, uint32_t virtual);
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, uint32_t virtual);
void free_page_tables(page_dir_entry_t *vm);
void __set_translation_table_base(uint64_t);
void __flush_tlb(void);

#endif
