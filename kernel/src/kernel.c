#include <mmu.h>
#include <kernel.h>
#include <hardware.h>
#include <kalloc.h>
#include <system.h>
#include <console.h>
#include <proc.h>

#include <lib/string.h>


/* kernel virtual to physical memory mappings */
static MemoryMapT _kernelMaps[] = {
	{KERNEL_BASE, 0, V2P(_kernelEnd), AP_RW_D},
	{MMIO_BASE, MMIO_BASE_PHY, MMIO_BASE_PHY + MMIO_MEM_SIZE, AP_RW_D},
	{INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D},
	{ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START),
		0, AP_RW_D}
};

PageDirEntryT* _kernelVM = NULL;

void initKernelVM() 
{
	_kernelMaps[3].pEnd = getPhyRamSize();

	//get the end of bootup part kernel.
	unsigned kend = (unsigned)_kernelEnd; 
	//align up to PAGE_DIR_SIZE (like 16KB in this case).
	_kernelVM = (PageDirEntryT*)ALIGN_UP(kend, PAGE_DIR_SIZE);

	setKernelVM(_kernelVM);
	
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	__setTranslationTableBase(V2P((uint32_t)_kernelVM));
}

void setKernelVM(PageDirEntryT* vm) 
{
	memset(vm, 0, PAGE_DIR_SIZE);

	// add each of the mappings
	for (int i = 0; i < 4; i++) {
		mapPages(vm, _kernelMaps[i]);
	}
}

void schedulerInit();
void schedule();
bool loadInit(ProcessT *proc);

void kernelEntry() 
{
	/*
	build free mems list only for kernel init.

	We can only use init memory part
	(from ALLOCATABLE_MEMORY_START to 'KERNEL_BASE + INIT_MEMORY_SIZE'),
	cause the boot program only mapped part of mem by _startupPageDir(startup.c).
	Notice: This part of physical mem (0 to INIT_MEMORY_SIZE) 
		only works for init kernel page mapping,
		means, after that, it can not be used any more!
	*/
	kallocInit(ALLOCATABLE_MEMORY_START,
			KERNEL_BASE + INIT_MEMORY_SIZE);

	/*
	Done mapping all mem
	*/
	initKernelVM();

	/*
	Since kernel mem mapping finished, 
	we can build free mems list for all the rest mem
	Notice:	From now, you can only use physical mem from 
		INIT_MEMORY_SIZE to getPhyRamSize.
	*/
	kallocInit(KERNEL_BASE + INIT_MEMORY_SIZE,
			KERNEL_BASE + getPhyRamSize());

	consoleInit();
	kputs("\n\n=================\n"
				"EwokOS (by Misa.Z)\n"
				"=================\n");

	kputs("MMU got ready.\n");

	procInit();
	kputs("Process manager got ready.\nLoading the first process...\n");

	ProcessT *proc = procCreate();
	loadInit(proc);
	schedulerInit();
	schedule();
}
