#include <mmu.h>
#include <kernel.h>
#include <hardware.h>
#include <kalloc.h>
#include <system.h>
#include <dev/uart.h>
#include <proc.h>
#include <lib/string.h>

PageDirEntryT* _kernelVM = NULL;

void initKernelVM() 
{

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

	mapPages(vm, KERNEL_BASE, 0, V2P(_kernelEnd), AP_RW_D);
	/*for ramdisk*/
	mapPages(vm, INITRD_BASE, INITRD_BASE, INITRD_BASE+INITRD_SIZE, AP_RW_D);
	mapPages(vm, INITRD_NEW_BASE, V2P(INITRD_NEW_BASE), V2P(INITRD_NEW_BASE+INITRD_SIZE), AP_RW_D);

	mapPages(vm, MMIO_BASE, MMIO_BASE_PHY, MMIO_BASE_PHY + MMIO_MEM_SIZE, AP_RW_D);
	mapPages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D);
	mapPages(vm, ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START), getPhyRamSize(), AP_RW_D);
}

void schedulerInit();
void schedule();
bool loadInit(ProcessT *proc);

char* _initRamDiskBase = 0;

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
	move ramdisk to high memory.
	*/
	_initRamDiskBase = (void*)(INITRD_NEW_BASE);
	memcpy(_initRamDiskBase, (void*)(INITRD_BASE), INITRD_SIZE);

	/*
	Since kernel mem mapping finished, 
	we can build free mems list for all the rest mem
	Notice:	From now, you can only kalloc physical mem from 
		INIT_MEMORY_SIZE to getPhyRamSize.
	*/
	kallocInit(KERNEL_BASE + INIT_MEMORY_SIZE,
			KERNEL_BASE + getPhyRamSize());

	procInit();

	uartInit();
	uartPuts("\n\n=================\n"
				"EwokOS (by Misa.Z)\n"
				"=================\n"
				"Kernel got ready(MMU and ProcMan).\n"
				"Loading the first process...\n");

	ProcessT *proc = procCreate();
	loadInit(proc);

	schedulerInit();
	schedule();
}
