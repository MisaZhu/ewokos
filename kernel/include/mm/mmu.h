#ifndef MMU_H
#define MMU_H

#include <types.h>

#define KB 1024
#define MB (1024*KB)
#define GB (1024*MB)

#define PAGE_SIZE (4*KB)
#define PAGE_TABLE_SIZE (1*KB)

#define PAGE_DIR_NUM 4096
#define PAGE_DIR_SIZE (PAGE_DIR_NUM*4)

#define KERNEL_BASE                    0x80000000 //=2G virtual address start base.
#define MMIO_BASE                      (KERNEL_BASE + 1*GB)
#define INTERRUPT_VECTOR_BASE          0xffff0000
#define USER_STACK_TOP                 (KERNEL_BASE - PAGE_SIZE)

#define KERNEL_PAGE_DIR_BASE           ALIGN_UP((uint32_t)_kernel_end, PAGE_DIR_SIZE)
#define KERNEL_PAGE_DIR_END            (KERNEL_PAGE_DIR_BASE + 128*KB)

#define KMALLOC_BASE                   ALIGN_UP(KERNEL_PAGE_DIR_END, PAGE_SIZE)
#define KMALLOC_END                    (KMALLOC_BASE + 32*MB)

#define ALLOCATABLE_PAGE_DIR_BASE      KMALLOC_END
#define ALLOCATABLE_PAGE_DIR_END       (ALLOCATABLE_PAGE_DIR_BASE + 1*MB)

#define ALLOCATABLE_MEMORY_START       ALLOCATABLE_PAGE_DIR_END


#define V2P(V) ((uint32_t)V - KERNEL_BASE)
#define P2V(P) ((uint32_t)P + KERNEL_BASE)

#define ALIGN_DOWN(x, alignment) ((x) & ~(alignment - 1))
#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

#define get32(addr) (*((volatile uint32_t *)(addr)))
#define put32(addr, val) (*((volatile uint32_t *)(addr)) = (val))
#define get8(addr) (*((volatile uint8_t *)(addr)))
#define put8(addr, val) (*((volatile uint8_t *)(addr)) = (val))

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

extern unsigned _startup_page_dir[PAGE_DIR_NUM];
extern uint32_t _mmio_base;

#endif
