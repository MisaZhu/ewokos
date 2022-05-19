#ifndef MMU_DEF_H
#define MMU_DEF_H

#include <stdint.h>

#define KB 1024
#define MB (1024*KB)
#define GB (1024*MB)

#define PAGE_SIZE (4*KB)
#define PAGE_TABLE_SIZE (1*KB)

#define PAGE_DIR_NUM 4096
#define PAGE_DIR_SIZE (PAGE_DIR_NUM*4)

#define KERNEL_BASE                    0x80000000 //=2G virtual address start base.
#define INTERRUPT_VECTOR_BASE          0xffff0000

#define MMIO_BASE                      (KERNEL_BASE + 1*GB)
#define USER_STACK_TOP                 (KERNEL_BASE - PAGE_SIZE)

#define KERNEL_IMAGE_END               ALIGN_UP((uint32_t)_kernel_end, PAGE_DIR_SIZE)

#define KERNEL_PAGE_DIR_BASE           KERNEL_IMAGE_END
#define KERNEL_PAGE_DIR_END            (KERNEL_PAGE_DIR_BASE + 128*KB)

#define KMALLOC_BASE                   ALIGN_UP(KERNEL_PAGE_DIR_END, PAGE_SIZE)
#define KMALLOC_END                    (KMALLOC_BASE + 8*MB)

#define ALLOCATABLE_PAGE_DIR_BASE      KMALLOC_END
#define ALLOCATABLE_PAGE_DIR_END       (ALLOCATABLE_PAGE_DIR_BASE + 1*MB)

#define ALLOCATABLE_MEMORY_START       ALLOCATABLE_PAGE_DIR_END

#define ALIGN_DOWN(x, alignment) ((x) & ~(alignment - 1))
#define ALIGN_UP(x, alignment) (((x) + alignment - 1) & ~(alignment - 1))

#define get32(addr) (*((volatile uint32_t *)(addr)))
#define put32(addr, val) (*((volatile uint32_t *)(addr)) = (val))
#define get8(addr) (*((volatile uint8_t *)(addr)))
#define put8(addr, val) (*((volatile uint8_t *)(addr)) = (val))

/* descriptor types */
#define SMALL_PAGE_TYPE 2
#define PAGE_DIR_2LEVEL_TYPE 1

/* access permissions */
#define AP_RW_D  0x1
#define AP_RW_R  0x2
#define AP_RW_RW 0x3

#endif
