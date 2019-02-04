#ifndef KMALLOC_H
#define KMALLOC_H

/*kmalloc/kmfree manage the kernel-reserved memory trunk(from KMALLOC_BASE, with size KMALLOC_SIZE), the memory trunk only reserved for kernel, not managed by MMU! */

#include <types.h>

void* kmalloc(uint32_t size);
void kmfree(void* p);
void kmInit();

#endif
