#ifndef MMU_DEF_H
#define MMU_DEF_H

#include <stdint.h>

#define KB 1024
#define MB (1024*KB)
#define GB (1024*MB)

#ifdef PAGE_SIZE_16K
#define PAGE_SHIFT 14
#define PAGE_SIZE (16*KB)
#define PAGE_TABLE_SIZE (16*KB)
#else
#define PAGE_SHIFT 12
#define PAGE_SIZE (4*KB)
#define PAGE_TABLE_SIZE (4*KB)
#endif

#define PAGE_DIR_NUM (PAGE_TABLE_SIZE / sizeof(uint64_t))
#define PAGE_DIR_SIZE (PAGE_DIR_NUM*8)

#define PAGE_LEVEL_BITS ((PAGE_SHIFT == 14) ? 11 : 9)
#define PAGE_L3_SHIFT PAGE_SHIFT
#define PAGE_L2_SHIFT (PAGE_L3_SHIFT + PAGE_LEVEL_BITS)
#define PAGE_L1_SHIFT (PAGE_L2_SHIFT + PAGE_LEVEL_BITS)

#define PAGE_TABLE_SPAN_L3 ((uint64_t)PAGE_SIZE * PAGE_DIR_NUM)
#define PAGE_TABLE_SPAN_L2 (PAGE_TABLE_SPAN_L3 * PAGE_DIR_NUM)
#define PAGE_BLOCK_SHIFT PAGE_L2_SHIFT
#define PAGE_BLOCK_SIZE  (1ull << PAGE_BLOCK_SHIFT)

#define KERNEL_BASE                    0x4000000000ull //=256G virtual address start base.

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
