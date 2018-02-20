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


void initKernelVM() 
{
	_kernelMaps[3].pEnd = getPhyRamSize();

	//get the end of bootup part kernel.
	unsigned kend = (unsigned)_kernelEnd; 
	//align up to PAGE_DIR_SIZE (like 16KB in this case).
	PageDirEntryT* vm = (PageDirEntryT*)ALIGN_UP(kend, PAGE_DIR_SIZE);

	setKernelVM(vm);
	
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	__setTranslationTableBase(V2P((uint32_t)vm));
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
	kallocInit(ALLOCATABLE_MEMORY_START,
			KERNEL_BASE + INIT_MEMORY_SIZE);

	initKernelVM();

	kallocInit(KERNEL_BASE + INIT_MEMORY_SIZE,
			KERNEL_BASE + getPhyRamSize());

	consoleInit();
	kputs("\n\n=================\nEwokOS booted.\n=================\n");

	procInit();
	ProcessT *proc = procCreate();
	loadInit(proc);

	schedulerInit();
	schedule();
}
