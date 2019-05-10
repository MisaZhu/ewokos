#ifndef KMALLOC_H
#define KMALLOC_H

/*km_alloc/km_free manage the kernel-reserved memory trunk(from KMALLOC_BASE, with size KMALLOC_SIZE), the memory trunk only reserved for kernel, not managed by MMU! */

#include <types.h>

void* km_alloc(uint32_t size);
void km_free(void* p);
void km_init(void);

mem_funcs_t* kmem_funcs(void);
#define KMFS kmem_funcs()

#endif
