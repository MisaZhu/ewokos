#ifndef KERNEL_H
#define KERNEL_H

#include <mmu.h>
#include <kramdisk.h>

extern char _kernelStart[];
extern char _kernelEnd[];

extern char* _initRamDiskBase;
extern RamDiskT _initRamDisk;

extern PageDirEntryT* _kernelVM;
extern void setKernelVM(PageDirEntryT* vm);


#endif
