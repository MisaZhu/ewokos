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
#include <dev/gpio.h>
#include <base16.h>
#include <dev/fb.h>
#include <dev/initrd.h>
#include <sramdisk.h>
#include <printk.h>

page_dir_entry_t* _kernel_vm;
uint32_t _phyMemSize = 0;

void init_kernel_vm() 
{
	/*
	build free mems list only for kernel init.
	We can only use init memory part
	(from ALLOCATABLE_MEMORY_START to 'KERNEL_BASE + INIT_MEMORY_SIZE'),
	cause the boot program only mapped part of mem by _startup_page_dir(startup.c).
	Notice: This part of physical mem (0 to INIT_MEMORY_SIZE) works for init kernel page mapping
	*/
	kalloc_init(ALLOCATABLE_MEMORY_START,
			ALLOCATABLE_MEMORY_START + INIT_RESERV_MEMORY_SIZE);


	//align up to PAGE_DIR_SIZE (like 16KB in this case). 16KB memory after kernel be used for kernel page dir table 
	_kernel_vm = (page_dir_entry_t*)ALIGN_UP((uint32_t)_kernel_end, PAGE_DIR_SIZE);

	set_kernel_vm(_kernel_vm);
	
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	__set_translation_table_base(V2P((uint32_t)_kernel_vm));

	/*
	init kernel trunk memory malloc.
	*/
	km_init();
}

void set_kernel_vm(page_dir_entry_t* vm) 
{
	memset(vm, 0, PAGE_DIR_SIZE);

	map_pages(vm, 0, 0, PAGE_SIZE, AP_RW_D);
	//map interrupt vector to high(virtual) mem
	map_pages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D);
	//map kernel image to high(virtual) mem
	map_pages(vm, KERNEL_BASE+PAGE_SIZE, PAGE_SIZE, V2P(_kernel_end), AP_RW_D);

	map_pages(vm, (uint32_t)_kernel_vm, V2P(_kernel_vm), V2P(_kernel_vm)+PAGE_DIR_SIZE, AP_RW_D);

	//map MMIO to high(virtual) mem.
	map_pages(vm, MMIO_BASE, get_mmio_base_phy(), get_mmio_base_phy() + get_mmio_mem_size(), AP_RW_D);
	
	//map kernel memory trunk to high(virtual) mem.
	map_pages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_BASE+KMALLOC_SIZE), AP_RW_D);

	if(_phyMemSize == 0) //map some allocable memory for the pagetable alloc for rest momory mapping(coz we don't know the whole phymem size yet.
		map_pages(vm, ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START), V2P(ALLOCATABLE_MEMORY_START) + INIT_RESERV_MEMORY_SIZE, AP_RW_D);
	else
		map_pages(vm, ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START), _phyMemSize, AP_RW_D);

	arch_set_kernel_vm(vm);
}

static void init_allocable_mem() {
	_phyMemSize = get_phy_ram_size();
	/*
	Since kernel mem mapping finished, and init ram disk copied to kernel trunk memory.
	we can build free mem page list for all the rest mem(the init ram disk part can be reused as well).
	Notice:	From now, you can kalloc all the rest of physical mem.
	*/
	map_pages(_kernel_vm, ALLOCATABLE_MEMORY_START, V2P(ALLOCATABLE_MEMORY_START), _phyMemSize, AP_RW_D);
}

void load_init_proc() {
	const char* name = "init";

	printk("Loading the first process ... ");
	process_t *proc = proc_create(); //create first process
	if(proc == NULL) {
		printk("panic: init process create failed!\n");
		return;
	}
	
	int32_t size = 0;
	const char *p = read_initrd(name, &size);
	if(p == NULL) {
		printk("panic: init process load failed!\n");
		return;
	}
	else {
		proc_load(proc, p, size);
		strncpy(proc->cmd, name, CMD_MAX);
	}
	printk("ok.\n");
}

void kernel_entry() {
	/* Done mapping all mem */
	init_kernel_vm();

	/*init the rest allocable memory VM*/
	init_allocable_mem();

	/*hardware init*/
	hw_init();

	printk("\n=================\n"
				"EwokOS (by Misa.Z)\n"
				"=================\n"
				"Kernel got ready(MMU and ProcMan).\n\n");

	/*init share memory*/
	shm_init();

	/*init gpio*/
	gpio_init();

	/*framebuffer init*/
	_fb_init();

	/*init uart for output.*/
	uart_init(); 

	/*init ipc*/
	ipc_init();

	/*decode initramdisk to high memory(kernel trunk memory).*/
	printk("Load init ramdisk ... ");
	if(load_initrd() != 0)
		return;
	printk("ok.\n");
	
	/*init process mananer*/
	proc_init();

	/*load init process(first process)*/
	load_init_proc();

	/*init irq*/
	irq_init();

	/*timer irq*/
	timer_init();

	/*start scheduler*/
	scheduler_init();

	while(1) {
		schedule();
	}

	close_initrd();
}
