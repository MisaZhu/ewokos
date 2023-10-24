#ifndef MMU_DEF_H
#define MMU_DEF_H

#include <stdint.h>

#define KB 1024
#define MB (1024*KB)
#define GB (1024*MB)

#define PAGE_SIZE (4*KB)
#define PAGE_TABLE_SIZE (4*KB)

#define PAGE_DIR_NUM 512
#define PAGE_DIR_SIZE (PAGE_DIR_NUM*8)

#define KERNEL_BASE                    0x80000000 //=2G virtual address start base.
#define INTERRUPT_VECTOR_BASE          0xffff0000

//#define MAX_MEM_SIZE                   (1*GB + 512*MB) //max usable memory for 32bits OS
#define MAX_MEM_SIZE                   (1*GB) //max usable memory for 32bits OS

#define MMIO_BASE                      (KERNEL_BASE + MAX_MEM_SIZE)
#define USER_STACK_TOP                 (KERNEL_BASE - PAGE_SIZE)

#define KERNEL_IMAGE_END               ALIGN_UP((uint32_t)_kernel_end, PAGE_DIR_SIZE)

#define KERNEL_PAGE_DIR_BASE           KERNEL_IMAGE_END
#define KERNEL_PAGE_DIR_END            (KERNEL_PAGE_DIR_BASE + 1*MB)

#define KMALLOC_BASE                   ALIGN_UP(KERNEL_PAGE_DIR_END, PAGE_SIZE)
#define KMALLOC_END                    (KMALLOC_BASE + 8*MB)

#define ALLOCATABLE_PAGE_DIR_BASE      KMALLOC_END
#define ALLOCATABLE_PAGE_DIR_END       (ALLOCATABLE_PAGE_DIR_BASE + 4*MB)

#define ALLOCATABLE_MEMORY_START       ALLOCATABLE_PAGE_DIR_END

#define ALIGN_DOWN(x, alignment) ((x) & ~(alignment - 1))
#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

#define get64(addr) (*((volatile uint64_t *)(addr)))
#define put64(addr, val) (*((volatile uint64_t *)(addr)) = (uint64_t)(val))
#define get32(addr) (*((volatile uint32_t *)(addr)))
#define put32(addr, val) (*((volatile uint32_t *)(addr)) = (uint32_t)(val))
#define get16(addr) (*((volatile uint16_t *)(addr)))
#define put16(addr, val) (*((volatile uint16_t *)(addr)) = (uint16_t)(val))
#define get8(addr) (*((volatile uint8_t *)(addr)))
#define put8(addr, val) (*((volatile uint8_t *)(addr)) = (uint8_t)(val))

/* descriptor types */
#define SMALL_PAGE_TYPE 2
#define PAGE_DIR_2LEVEL_TYPE 1

/* access permissions */
#define AP_RW_D  0x1
#define AP_RW_R  0x2
#define AP_RW_RW 0x3

#endif
