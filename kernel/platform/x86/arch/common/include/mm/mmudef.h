#ifndef MMU_DEF_H
#define MMU_DEF_H

#include <stdint.h>

#define KB 1024
#define MB (1024 * KB)
#define GB (1024 * MB)

#define PAGE_SIZE       (4 * KB)
#define PAGE_TABLE_SIZE (4 * KB)

#define PAGE_DIR_NUM  512
#define PAGE_DIR_SIZE (PAGE_DIR_NUM * sizeof(uint64_t))

#define KERNEL_BASE           0x80000000
#define INTERRUPT_VECTOR_BASE 0xffff0000

#define MAX_USABLE_MEM_SIZE (512 * MB)

#define AP_RW_D  0x1
#define AP_RW_R  0x2
#define AP_RW_RW 0x3

#endif
