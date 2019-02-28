#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <hardware.h>
#include <kernel.h>
#include <system.h>
#include <dev/uart.h>
#include <proc.h>
#include <irq.h>
#include <kstring.h>
#include <timer.h>
#include <scheduler.h>

PageDirEntryT* _kernelVM;

void initKernelVM() 
{
	/*
	build free mems list only for kernel init.
	We can only use init memory part
	(from ALLOCATABLE_MEMORY_START to 'KERNEL_BASE + INIT_MEMORY_SIZE'),
	cause the boot program only mapped part of mem by _startupPageDir(startup.c).
	Notice: This part of physical mem (0 to INIT_MEMORY_SIZE) works for init kernel page mapping
	*/
	kallocInit(ALLOCATABLE_MEMORY_START,
			KERNEL_BASE + INIT_MEMORY_SIZE);


	//align up to PAGE_DIR_SIZE (like 16KB in this case). 16KB memory after kernel be used for kernel page dir table 
	_kernelVM = (PageDirEntryT*)ALIGN_UP((uint32_t)_kernelEnd, PAGE_DIR_SIZE);

	setKernelVM(_kernelVM);
	
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	__setTranslationTableBase(V2P((uint32_t)_kernelVM));

	/*
	init kernel trunk memory malloc.
	*/
	kmInit();

	/*
	Since kernel mem mapping finished, and init ram disk copied to kernel trunk memory.
	we can build free mem page list for all the rest mem(the init ram disk part can be reused as well).
	Notice:	From now, you can kalloc all the rest of physical mem.
	*/
	kallocInit(KERNEL_BASE + INIT_MEMORY_SIZE,
			KERNEL_BASE + getPhyRamSize());
}

void setKernelVM(PageDirEntryT* vm) 
{
	memset(vm, 0, PAGE_DIR_SIZE);

	mapPages(vm, 0, 0, PAGE_SIZE, AP_RW_D);
	//map interrupt vector to high(virtual) mem
	mapPages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D);
	//map kernel image to high(virtual) mem
	mapPages(vm, KERNEL_BASE+PAGE_SIZE, PAGE_SIZE, V2P(_kernelEnd), AP_RW_D);

	//map MMIO to high(virtual) mem.
	mapPages(vm, MMIO_BASE, getMMIOBasePhy(), getMMIOBasePhy() + getMMIOMemSize(), AP_RW_D);

	//map kernel memory trunk to high(virtual) mem.
	mapPages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_BASE+KMALLOC_SIZE), AP_RW_D);
	
	//map whole rest allocatable memory to high(virtual) mem.
	mapPages(vm, ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START), getPhyRamSize(), AP_RW_D);
}

#define FIRST_PROCESS "init"

void kernelEntry() 
{
	/* Done mapping all mem */
	initKernelVM();

	/*init uart for output.*/
	uartInit(); 

	/*decode initramdisk to high memory(kernel trunk memory).*/
	_initRamDiskBase = decodeInitFS();
	if(_initRamDiskBase == NULL) {
		uartPuts("panic: initramdisk decode failed!\n");
		return;
	}

	uartPuts("\n\n=================\n"
				"EwokOS (by Misa.Z)\n"
				"=================\n"
				"Kernel got ready(MMU and ProcMan).\n"
				"Loading the first process...\n\n");
	
	ipcInit();
	
	ramdiskOpen((const char*)_initRamDiskBase, &_initRamDisk, kmalloc);

	procInit();

	ProcessT *proc = procCreate(); //create first process
	if(proc == NULL) {
		uartPuts("panic: init process create failed!\n");
		return;
	}

	int size = 0;
	const char *p = ramdiskRead(&_initRamDisk, FIRST_PROCESS, &size);
	if(p == NULL) {
		uartPuts("panic: init process load failed!\n");
		return;
	}
	else {
		procLoad(proc, p);
		strncpy(proc->cmd, FIRST_PROCESS, CMD_MAX);
	}

	irqInit();
	timerInit();

	schedulerInit();

	//kramdiskClose(_initRamDisk, kmfree);
	//kmfree(_initRamDiskBase);
	while(1);
}
