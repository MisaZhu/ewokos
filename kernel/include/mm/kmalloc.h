#ifndef KMALLOC_H
#define KMALLOC_H

/*km_alloc/km_free manage the kernel-reserved memory trunk(from KMALLOC_BASE, with size KMALLOC_SIZE), the memory trunk only reserved for kernel, not managed by MMU! */

#include <types.h>

void* kmalloc(uint32_t size);
void kfree(void* p);
void* krealloc_raw(void* s, uint32_t old_size, uint32_t new_size);
void km_init(void);

#endif
