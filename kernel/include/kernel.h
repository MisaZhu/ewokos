#ifndef KERNEL_H
#define KERNEL_H

#include <mmu.h>

extern char _kernelStart[];
extern char _kernelEnd[];

extern void setKernelVM(PageDirEntryT* vm);

#endif
