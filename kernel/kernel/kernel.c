#include <dev/kdevice.h>
#include <dev/actled.h>
#include <dev/framebuffer.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/shm.h>
#include <kstring.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <kernel/hw_info.h>
#include <kernel/proc.h>
#include <kernel/irq.h>
#include <kernel/schedule.h>
#include <dev/timer.h>
#include <kprintf.h>
#include <vfs.h>
#include <dev/uart.h>
#include <ext2read.h>
#include <partition.h>
#include <basic_math.h>
#include <graph.h>

page_dir_entry_t* _kernel_vm = NULL;
uint32_t _mmio_base = 0;

static void set_kernel_init_vm(page_dir_entry_t* vm) {
	memset(vm, 0, PAGE_DIR_SIZE);

	//map interrupt vector to high(virtual) mem
	map_pages(vm, 0, 0, PAGE_SIZE, AP_RW_D);
	map_pages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D);

	//map kernel image, page dir, kernel malloc mem
	map_pages(vm, KERNEL_BASE+PAGE_SIZE, PAGE_SIZE, V2P(ALLOCATABLE_PAGE_DIR_END), AP_RW_D);

	//map MMIO to high(virtual) mem.
	hw_info_t* hw_info = get_hw_info();
	map_pages(vm, MMIO_BASE, hw_info->phy_mmio_base, hw_info->phy_mmio_base + hw_info->mmio_size, AP_RW_D);
	arch_vm(vm);
}

void set_kernel_vm(page_dir_entry_t* vm) {
	set_kernel_init_vm(vm);
	map_pages(vm, 
		ALLOCATABLE_MEMORY_START, 
		V2P(ALLOCATABLE_MEMORY_START),
		get_hw_info()->phy_mem_size,
		AP_RW_D);
}

static void init_kernel_vm(void) {
	for(int32_t i=0; i<RAM_HOLE_MAX; i++)
		_ram_holes[i].base = _ram_holes[i].end = 0;

	_kernel_vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
	//get kalloc ready just for kernel page tables.
	kalloc_init(KERNEL_PAGE_DIR_BASE+PAGE_DIR_SIZE, KERNEL_PAGE_DIR_END, false); 
	set_kernel_init_vm(_kernel_vm);

	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	__set_translation_table_base(V2P((uint32_t)_kernel_vm));
	_mmio_base = MMIO_BASE;
}

static void init_allocable_mem(void) {
	kalloc_init(ALLOCATABLE_PAGE_DIR_BASE, ALLOCATABLE_PAGE_DIR_END, false);
	map_pages(_kernel_vm,
		ALLOCATABLE_MEMORY_START,
		V2P(ALLOCATABLE_MEMORY_START),
		get_hw_info()->phy_mem_size,
		AP_RW_D);

	kalloc_init(ALLOCATABLE_MEMORY_START, P2V(get_hw_info()->phy_mem_size-32*MB), true);
}

static int32_t load_init(void) {
	const char* prog = "/sbin/init";
	int32_t sz;

	char* elf = sd_read_ext2(prog, &sz);
	if(elf != NULL) {
		proc_t *proc = proc_create(PROC_TYPE_PROC, NULL);
		str_cpy(proc->cmd, prog);
		int32_t res = proc_load_elf(proc, elf, sz);
		kfree(elf);
		return res;
	}
	return -1;
}

static void fs_init(void) {
	vfs_init();
}

void _kernel_entry_c(context_t* ctx) {
	(void)ctx;
	hw_info_init();
	init_kernel_vm();  
	km_init();

	uart_dev_init();
	init_console();

	printf("\n\n"
			"===Ewok micro-kernel===\n\n"
			"kernel: mmu inited\n"
			"kernel: uart inited\n");
	printf("kernel: kmalloc initing  [ok] : %dMB\n", div_u32(KMALLOC_END-KMALLOC_BASE, 1*MB));

	printf("kernel: framebuffer initing\n");
	if(fb_dev_init(1280, 720, 16) == 0) {
		fbinfo_t* info = fb_get_info();
		printf(  "[OK] : %dx%d %dbits, addr: 0x%X, size:%d\n", 
				info->width, info->height, info->depth,
				info->pointer, info->size);
		memset((void*)info->pointer, 0, info->size);
	}
	else {
		printf("  [Failed!]\n");
	}

	setup_console(); 

	init_allocable_mem(); //init the rest allocable memory VM
	printf("kernel: init allocable memory: %dMB\n", div_u32(get_free_mem_size(), 1*MB));

	printf("kernel: devices initing.\n");
	dev_init();
	
	hw_optimise();

	init_global();
	printf("kernel: global env inited.\n");

	shm_init();
	printf("kernel: share memory inited.\n");

	procs_init();
	printf("kernel: processes inited.\n");

	fs_init();
	printf("kernel: vfs inited\n");

	irq_init();
	printf("kernel: irq inited\n");

	printf("kernel: loading init");
	if(load_init() != 0) 
		printf(" [failed!]\n");
	else
		printf(" [ok]\n");
	
	printf("kernel: start timer.\n");
	timer_set_interval(0, 0x200); 

	while(1) {
		__asm__("MOV r0, #0; MCR p15,0,R0,c7,c0,4"); // CPU enter WFI state
	}

	close_console();
}
