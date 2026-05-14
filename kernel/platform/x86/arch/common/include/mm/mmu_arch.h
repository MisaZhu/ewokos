#ifndef MMU_ARCH_H
#define MMU_ARCH_H

#include <stdint.h>
#include <mm/mmudef.h>

#define PAGE_PML4_INDEX(x) (((x) >> 39) & 0x1FF)
#define PAGE_PDPT_INDEX(x) (((x) >> 30) & 0x1FF)
#define PAGE_PD_INDEX(x)   (((x) >> 21) & 0x1FF)
#define PAGE_PT_INDEX(x)   (((x) >> 12) & 0x1FF)

typedef union {
	struct {
		uint64_t present : 1;
		uint64_t rw : 1;
		uint64_t us : 1;
		uint64_t pwt : 1;
		uint64_t pcd : 1;
		uint64_t accessed : 1;
		uint64_t dirty : 1;
		uint64_t pat : 1;
		uint64_t global : 1;
		uint64_t ignored0 : 3;
		uint64_t Address : 40;
		uint64_t ignored1 : 11;
		uint64_t nx : 1;
	};
	uint64_t value;
} page_table_entry_t;

typedef page_table_entry_t page_dir_entry_t;

#define PTE_ATTR_WRBACK          0
#define PTE_ATTR_DEV             1
#define PTE_ATTR_WRTHR           2
#define PTE_ATTR_WRBACK_ALLOCATE 3
#define PTE_ATTR_STRONG_ORDER    4
#define PTE_ATTR_NOCACHE         5

void set_pte_flags(page_table_entry_t* pte, uint32_t pte_attr);
page_table_entry_t* get_page_table_entry(page_dir_entry_t* vm, uint32_t virtual_addr);
void __set_translation_table_base(uint64_t base);
void __flush_tlb(void);

#endif
