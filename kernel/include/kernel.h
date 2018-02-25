#ifndef KERNEL_H
#define KERNEL_H

#include <mmu.h>

extern char _kernelStart[];
extern char _kernelEnd[];
extern char* _initRamDiskBase;

extern PageDirEntryT* _kernelVM;
extern void setKernelVM(PageDirEntryT* vm);


#endif
