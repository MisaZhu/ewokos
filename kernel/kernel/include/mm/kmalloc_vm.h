#ifndef KMALLOC_VM_H
#define KMALLOC_VM_H

/*km_alloc/km_free manage the kernel-reserved memory trunk(from KMALLOC_BASE, with size KMALLOC_SIZE), the memory trunk only reserved for kernel, not managed by MMU! */

#include <stdint.h>
#include <mm/mmu.h>

void  kmalloc_vm_init(void);
void* kmalloc_vm(page_dir_entry_t *vm, uint32_t size);
void  kfree_vm(page_dir_entry_t *vm, void* p);

#endif
