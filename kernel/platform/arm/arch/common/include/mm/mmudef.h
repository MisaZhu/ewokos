#ifndef MMU_DEF_H
#define MMU_DEF_H

#include <stdint.h>
#include <ewokos_config.h>

#define KB 1024
#define MB (1024*KB)
#define GB (1024*MB)

#define PAGE_SIZE (4*KB)
#define PAGE_TABLE_SIZE (1*KB)

#define PAGE_DIR_NUM 4096
#define PAGE_DIR_SIZE (PAGE_DIR_NUM*4)

#define KERNEL_BASE                    0x80000000 //=2G virtual address start base.
#define INTERRUPT_VECTOR_BASE          0xffff0000

/* descriptor types */
#define SMALL_PAGE_TYPE 2
#define PAGE_DIR_2LEVEL_TYPE 1

/* access permissions */
#define AP_RW_D  0x1
#define AP_RW_R  0x2
#define AP_RW_RW 0x3

#endif
