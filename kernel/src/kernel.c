#include <printk.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/shm.h>
#include <hardware.h>
#include <kernel.h>
#include <system.h>
#include <dev/uart.h>
#include <proc.h>
#include <irq.h>
#include <kstring.h>
#include <timer.h>
#include <scheduler.h>
#include <dev/initfs.h>
#include <dev/gpio.h>
#include <base16.h>
#include <mailbox.h>
#include <dev/fb.h>
#include <printk.h>

PageDirEntryT* _kernelVM;
uint32_t _phyMemSize = 0;

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
			ALLOCATABLE_MEMORY_START + INIT_RESERV_MEMORY_SIZE);


	//align up to PAGE_DIR_SIZE (like 16KB in this case). 16KB memory after kernel be used for kernel page dir table 
	_kernelVM = (PageDirEntryT*)ALIGN_UP((uint32_t)_kernelEnd, PAGE_DIR_SIZE);

	setKernelVM(_kernelVM);
	
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	__setTranslationTableBase(V2P((uint32_t)_kernelVM));

	/*
	init kernel trunk memory malloc.
	*/
	kmInit();
}

void setKernelVM(PageDirEntryT* vm) 
{
	memset(vm, 0, PAGE_DIR_SIZE);

	mapPages(vm, 0, 0, PAGE_SIZE, AP_RW_D);
	//map interrupt vector to high(virtual) mem
	mapPages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D);
	//map kernel image to high(virtual) mem
	mapPages(vm, KERNEL_BASE+PAGE_SIZE, PAGE_SIZE, V2P(_kernelEnd), AP_RW_D);

	mapPages(vm, (uint32_t)_kernelVM, V2P(_kernelVM), V2P(_kernelVM)+PAGE_DIR_SIZE, AP_RW_D);

	//map MMIO to high(virtual) mem.
	mapPages(vm, MMIO_BASE, getMMIOBasePhy(), getMMIOBasePhy() + getMMIOMemSize(), AP_RW_D);
	
	//map kernel memory trunk to high(virtual) mem.
	mapPages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_BASE+KMALLOC_SIZE), AP_RW_D);

	if(_phyMemSize == 0) //map some allocable memory for the pagetable alloc for rest momory mapping(coz we don't know the whole phymem size yet.
		mapPages(vm, ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START), V2P(ALLOCATABLE_MEMORY_START) + INIT_RESERV_MEMORY_SIZE, AP_RW_D);
	else
		mapPages(vm, ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START), _phyMemSize, AP_RW_D);

	archSetKernelVM(vm);
}

static void initAllocableMem() {
	_phyMemSize = getPhyRamSize();
	/*
	Since kernel mem mapping finished, and init ram disk copied to kernel trunk memory.
	we can build free mem page list for all the rest mem(the init ram disk part can be reused as well).
	Notice:	From now, you can kalloc all the rest of physical mem.
	*/
	mapPages(_kernelVM, ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START), _phyMemSize, AP_RW_D);
}

char* _initRamDiskBase = 0;
uint32_t _initRamDiskSize = 0;
RamDiskT _initRamDisk;

char* decodeInitFS() {
	char* ret;
	char* p;
	int32_t i;
	int32_t sz;
	const char* s;

	ret = (char*)kmalloc(_initfsSize);
	if(ret == NULL)
		return NULL;

	p = ret;
	i = 0;
	while(1) {
		s = _initfs[i];
		if(s[0] == 0)
			break;

		base16Decode(s, strlen(s), p, &sz);
		p += sz;	
		i++;
	}
	_initRamDiskSize = _initfsSize;
	return ret;
}

#define FIRST_PROCESS "init"
void kernelEntry() {
	/* Done mapping all mem */
	initKernelVM();

	/*init the rest allocable memory VM*/
	initAllocableMem();

	printk("\n=================\n"
				"EwokOS (by Misa.Z)\n"
				"=================\n"
				"Kernel got ready(MMU and ProcMan).\n\n");

	/*init share memory*/
	shmInit();

	/*init gpio*/
	gpioInit();

	/*init mailbox for board support*/
	mailboxInit();

	/*framebuffer init*/
	fbInit();

	/*init uart for output.*/
	uartInit(); 

	/*init ipc*/
	ipcInit();

	/*decode initramdisk to high memory(kernel trunk memory).*/
	printk("Load init ramdisk ... ");
	_initRamDiskBase = decodeInitFS();
	if(_initRamDiskBase == NULL) {
		printk("panic: initramdisk decode failed!\n");
		return;
	}
	ramdiskOpen((const char*)_initRamDiskBase, &_initRamDisk, kmalloc);
	printk("ok.\n");

	/*init process mananer*/
	procInit();

	printk("Loading the first process ... ");
	ProcessT *proc = procCreate(); //create first process
	if(proc == NULL) {
		printk("panic: init process create failed!\n");
		return;
	}
	int size = 0;
	const char *p = ramdiskRead(&_initRamDisk, FIRST_PROCESS, &size);
	if(p == NULL) {
		printk("panic: init process load failed!\n");
		return;
	}
	else {
		procLoad(proc, p, size);
		strncpy(proc->cmd, FIRST_PROCESS, CMD_MAX);
	}
	printk("ok.\n");

	/*init irq*/
	irqInit();

	/*timer irq*/
	timerInit();

	/*start scheduler*/
	schedulerInit();

	while(1) {
		schedule();
	}

	ramdiskClose(&_initRamDisk, kmfree);
	kmfree(_initRamDiskBase);
}
