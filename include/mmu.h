#ifndef MMU_H
#define MMU_H

#define KB 1024
#define MB (1024*KB)

#define PAGE_SIZE (4*KB)

#define SECTION_TABLE_SIZE 4096
#define SECTION_TABLE_ALIGNMENT (16*KB)

#define KERNEL_BASE 0x80000000 //=2G

#endif
