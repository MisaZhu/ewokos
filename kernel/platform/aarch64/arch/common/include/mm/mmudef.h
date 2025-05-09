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

/* descriptor types */
#define TYPE_INVALID	0
#define TYPE_BLOCK		1
#define TYPE_PAGE       3
#define TYPE_TABLE		3

#define PAGE_DIR_2LEVEL_TYPE 1

/* access permissions */
#define AP_RW_D  0x0
#define AP_RW_RW 0x1
#define AP_RO_D  0x2
#define AP_RO_R  0x3
#define AP_RW_R	 AP_RO_R

#endif
