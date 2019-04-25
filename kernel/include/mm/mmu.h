#ifndef MMU_H
#define MMU_H

#include <types.h>

#define PAGE_SIZE (4*KB)
#define PAGE_TABLE_SIZE (1*KB)

#define PAGE_DIR_NUM 4096
#define PAGE_DIR_SIZE (PAGE_DIR_NUM*4)

#define KERNEL_BASE 0x80000000 //=2G virtual address start base.
#define MMIO_BASE (KERNEL_BASE + 1*GB)
#define IPC_TASK_BASE (MMIO_BASE + 512*MB)
#define INTERRUPT_VECTOR_BASE 0xffff0000

#define USER_STACK_BOTTOM (KERNEL_BASE - 2 * PAGE_SIZE)
//#define KERNEL_STACK_BOTTOM (KERNEL_BASE - 3 * PAGE_SIZE)

#define KMALLOC_BASE ((uint32_t)&_kernel_end +  256*KB) //256KB reserved for kernel pageDirTable(at least 16KB).
#define KMALLOC_SIZE 4*MB // keep for kernel trunk memory, can only used by kernel(km_alloc/km_free).

#define ALLOCATABLE_MEMORY_START (KMALLOC_BASE + KMALLOC_SIZE)

#define INIT_RESERV_MEMORY_SIZE (8*MB) 

#define V2P(V) ((uint32_t)V - KERNEL_BASE)
#define P2V(P) ((uint32_t)P + KERNEL_BASE)

#define get32(addr) (*((uint32_t *)(addr)))
#define put32(addr, val) (*((uint32_t *)(addr)) = (val))

#define mmio_read(reg) *((volatile uint32_t *)(reg))
#define mmio_write(reg, data) *((volatile uint32_t *)(reg)) = (data)

/* descriptor types */
#define PAGE_TYPE 2
#define PAGE_DIR_TYPE 1

/* access permissions */
#ifdef A_CORE
	#define AP_RW_D 0x5
	#define AP_RW_R 0xa
	#define AP_RW_RW 0xf
#else
	#define AP_RW_D 0x55
	#define AP_RW_R 0xaa
	#define AP_RW_RW 0xff
#endif

#define PAGE_DIR_INDEX(x) ((uint32_t)x >> 20)
#define PAGE_INDEX(x) (((uint32_t)x >> 12) & 255)

#define PAGE_TABLE_TO_BASE(x) ((uint32_t)x >> 10)
#define BASE_TO_PAGE_TABLE(x) ((void *) ((uint32_t)x << 10))
#define PAGE_TO_BASE(x) ((uint32_t)x >> 12)

#ifndef __ASSEMBLER__

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
	int access_permissions);

void map_page(page_dir_entry_t *vm, 
  uint32_t virtual_addr, 
	uint32_t physical,
	int access_permissions);

void unmap_page(page_dir_entry_t *vm, uint32_t virtual_addr);
void free_page_tables(page_dir_entry_t *vm);
uint32_t resolve_phy_address(page_dir_entry_t *vm, uint32_t virtual);
page_table_entry_t* get_page_table_entry(page_dir_entry_t *vm, uint32_t virtual);

#endif

#endif
