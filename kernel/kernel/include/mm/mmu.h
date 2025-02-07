#ifndef MMU_H
#define MMU_H

#include <mm/mmu_arch.h>
#include <kernel/hw_info.h>


#define ALIGN_DOWN(x, alignment) ((x) & ~(alignment - 1))
#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

#define V2P(V) ((uint32_t)(V) - KERNEL_BASE + _sys_info.phy_offset)
#define P2V(P) ((uint32_t)(P) + KERNEL_BASE - _sys_info.phy_offset)

#define KERNEL_IMAGE_END           ALIGN_UP((uint32_t)_kernel_end, PAGE_DIR_SIZE)

#define KERNEL_PAGE_DIR_BASE           KERNEL_IMAGE_END
#define KERNEL_PAGE_DIR_END            (KERNEL_PAGE_DIR_BASE + 256*KB)

#define ALLOCABLE_PAGE_DIR_BASE        KERNEL_PAGE_DIR_END
#define ALLOCABLE_PAGE_DIR_END         (ALLOCABLE_PAGE_DIR_BASE + 2*MB)

#define KERNEL_VSYSCALL_INFO_BASE          ALLOCABLE_PAGE_DIR_END
#define KERNEL_VSYSCALL_INFO_END           (KERNEL_VSYSCALL_INFO_BASE+4*KB)

#define KMALLOC_BASE                   KERNEL_VSYSCALL_INFO_END
#define KMALLOC_END                    (KMALLOC_BASE + get_kmalloc_size())

#define MMIO_BASE                      (KERNEL_BASE + MAX_MEM_SIZE)
#define MMIO_END                      (MMIO_BASE + _sys_info.mmio.size)

#define DMA_BASE                      (MMIO_END)
#define DMA_END                       (DMA_BASE+DMA_SIZE)

#define USER_STACK_TOP                 (KERNEL_BASE - PAGE_SIZE)
#define USER_STACK_MAX                 (64*MB)
#define USER_STACK_BOTTOM              (USER_STACK_TOP - USER_STACK_MAX)

#define get64(addr) (*((volatile uint64_t *)(addr)))
#define put64(addr, val) (*((volatile uint64_t *)(addr)) = (uint64_t)(val))
#define get32(addr) (*((volatile uint32_t *)(addr)))
#define put32(addr, val) (*((volatile uint32_t *)(addr)) = (uint32_t)(val))
#define get16(addr) (*((volatile uint16_t *)(addr)))
#define put16(addr, val) (*((volatile uint16_t *)(addr)) = (uint16_t)(val))
#define get8(addr) (*((volatile uint8_t *)(addr)))
#define put8(addr, val) (*((volatile uint8_t *)(addr)) = (uint8_t)(val))

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

void map_page_ref(page_dir_entry_t *vm, uint32_t vaddr, uint32_t paddr, uint32_t permissions, uint32_t pte_attr);
void unmap_page_ref(page_dir_entry_t *vm, uint32_t vaddr);

void free_page_tables(page_dir_entry_t *vm);
uint32_t resolve_kernel_address(page_dir_entry_t *vm, uint32_t virtual);

uint32_t get_allocable_start(void);

#endif
