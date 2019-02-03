#ifndef MMU_H
#define MMU_H

#include <types.h>

#define PAGE_SIZE (4*KB)
#define PAGE_TABLE_SIZE (1*KB)

#define PAGE_DIR_NUM 4096
#define PAGE_DIR_SIZE (PAGE_DIR_NUM*4)

#define KERNEL_BASE 0x80000000 //=2G virtual address start base.
#define MMIO_BASE (KERNEL_BASE + 1*GB)
#define INTERRUPT_VECTOR_BASE 0xffff0000

#define KERNEL_STACK_BOTTOM (KERNEL_BASE - 2 * PAGE_SIZE)
#define USER_STACK_BOTTOM (KERNEL_BASE - 3 * PAGE_SIZE)

#define KMALLOC_BASE ((uint32_t)_kernelEnd +  32*KB) //32KB reserved for kernel pageDirTable(at least 16KB).
#define KMALLOC_SIZE 2*MB
#define ALLOCATABLE_MEMORY_START (KMALLOC_BASE + KMALLOC_SIZE)

#define INIT_MEMORY_SIZE (8*MB) //must same as startup.c startuptable

//init ramdisk
#define INITRD_BASE 0x08000000 //=64M, qemu-system-arm -initrd <FILE> will load FILE to Physical memory address 64M when bootup. 
#define INITRD_SIZE 1*MB

#define ALIGN_DOWN(x, alignment) ((x) & ~(alignment - 1))
#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

#define V2P(V) ((uint32_t)V - KERNEL_BASE)
#define P2V(P) ((uint32_t)P + KERNEL_BASE)
#define MMIO_P2V(x) ((volatile unsigned int*) (MMIO_BASE + (x - MMIO_BASE_PHY)))

/* descriptor types */
#define PAGE_TYPE 2
#define PAGE_DIR_TYPE 1

/* access permissions */
#define AP_RW_D 0x55
#define AP_RW_R 0xaa
#define AP_RW_RW 0xff

#define PAGE_DIR_INDEX(x) ((uint32_t)x >> 20)
#define PAGE_INDEX(x) (((uint32_t)x >> 12) & 255)

#define PAGE_TABLE_TO_BASE(x) ((uint32_t)x >> 10)
#define BASE_TO_PAGE_TABLE(x) ((void *) ((uint32_t)x << 10))
#define PAGE_TO_BASE(x) ((uint32_t)x >> 12)

#ifndef __ASSEMBLER__

/* a 32-bit entry in hardware's PageDir table */
typedef struct {
	unsigned int type : 2;
	unsigned int : 3;
	unsigned int domain : 4;
	unsigned int : 1;
	unsigned int base : 22;
} PageDirEntryT;

/* a 32-bit entry in hardware's page table */
typedef struct {
	unsigned int type : 2;
	unsigned int bufferable : 1;
	unsigned int cacheable : 1;
	unsigned int permissions : 8;
	unsigned int base : 20;
} PageTableEntryT; 

void mapPages(PageDirEntryT *vm, uint32_t vaddr, 
	uint32_t pstart, 
	uint32_t pend,  
	int access_permissions);

void mapPage(PageDirEntryT *vm, 
  uint32_t virtual_addr, 
	uint32_t physical,
	int access_permissions);

void unmapPage(PageDirEntryT *vm, uint32_t virtual_addr);
void freePageTables(PageDirEntryT *vm);
uint32_t resolvePhyAddress(PageDirEntryT *vm, uint32_t virtual);

#endif

#endif
