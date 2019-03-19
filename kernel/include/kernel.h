#ifndef KERNEL_H
#define KERNEL_H

#include <mm/mmu.h>
#include <sramdisk.h>

extern char _kernelStart[];
extern char _kernelEnd[];
extern char _initStackTop[];
extern char _irqStackTop[];

extern uint32_t _phyMemSize;

extern PageDirEntryT* _kernelVM;
void setKernelVM(PageDirEntryT* vm);
void setAllocableVM(PageDirEntryT* vm);


#endif
