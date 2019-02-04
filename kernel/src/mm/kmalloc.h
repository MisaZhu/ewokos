#ifndef KMALLOC_H
#define KMALLOC_H

/*kmalloc/kmfree manage the kernel-reserved memory trunk, the memory trunk is from KMALLOC_BASE, with size KMALLOC_SIZE.*/

#include <types.h>

void* kmalloc(uint32_t size);
void kmfree(void* p);
void kmInit();

#endif
