#ifndef KMALLOC_H
#define KMALLOC_H

/*km_alloc/km_free manage the kernel-reserved memory trunk(from KMALLOC_BASE, with size KMALLOC_SIZE), the memory trunk only reserved for kernel, not managed by MMU! */

#include <stdint.h>
#include <mm/mmu.h>

void* kmalloc(uint32_t size);
void* kcalloc(uint32_t nmemb, uint32_t size);
void  kfree(void* p);
void  kmalloc_init(void);
uint32_t  kmalloc_free_size(void);

void* kmalloc_extra(page_dir_entry_t *vm, uint32_t size);
void  kfree_extra(page_dir_entry_t *vm, void* p);

#endif
