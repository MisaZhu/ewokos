#include <mmu.h>
#include <kernel.h>
#include <hardware.h>
#include <kalloc.h>
#include <kmalloc.h>
#include <system.h>
#include <dev/uart.h>
#include <proc.h>
#include <string.h>
#include <sramdisk.h>

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

	/*mapping ramdisk part*/
	mapPages(vm, INITRD_BASE, INITRD_BASE, INITRD_BASE+INITRD_SIZE, AP_RW_D);

	mapPages(vm, MMIO_BASE, MMIO_BASE_PHY, MMIO_BASE_PHY + MMIO_MEM_SIZE, AP_RW_D);
	mapPages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D);

	mapPages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_BASE+KMALLOC_SIZE), AP_RW_D);
	mapPages(vm, ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START), getPhyRamSize(), AP_RW_D);
}

void schedulerInit();
void schedule();
bool loadInit(ProcessT *proc);

char* _initRamDiskBase = 0;
RamDiskT _initRamDisk;

#define FIRST_PROCESS "kserv"

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
	init kernel malloc.
	*/
	kmInit();

	/*
	move ramdisk to high memory.
	*/
	_initRamDiskBase = kmalloc(INITRD_SIZE);
	memcpy(_initRamDiskBase, (void*)(INITRD_BASE), INITRD_SIZE);

	/*
	init ram disk
	*/
	ramdiskOpen((const char*)_initRamDiskBase, &_initRamDisk, kmalloc);

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
				"Loading the first process...\n\n");

	/*create first process*/
	ProcessT *proc = procCreate();
	
	/*load process from ramdisk by name.*/
	int size = 0;
	const char*p = ramdiskRead(&_initRamDisk, FIRST_PROCESS, &size);
	if(p != NULL)
		procLoad(proc, p);
	
	/*schedule processes*/
	schedulerInit();
	schedule();

	//kramdiskClose(_initRamDisk, kmfree);
	//kmfree(_initRamDiskBase);
	while(1);
}
