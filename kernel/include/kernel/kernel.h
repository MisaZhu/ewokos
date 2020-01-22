#ifndef KERNEL_H
#define KERNEL_H

#include <mm/mmu.h>

extern uint32_t _kernel_tic;
extern char _kernel_start[];
extern char _kernel_end[];

extern page_dir_entry_t* _kernel_vm;
extern void set_kernel_vm(page_dir_entry_t* vm);

#endif
