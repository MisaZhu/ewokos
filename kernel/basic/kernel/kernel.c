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
#include <kernel/kevqueue.h>
#include <dev/timer.h>
#include <kprintf.h>
#include <dev/uart.h>
#include <basic_math.h>
#include <stddef.h>

#ifdef SDC
#include <dev/sd.h>
#endif

#ifdef DISPLAY
#include <kconsole.h>
#endif

#ifdef FRAMEBUFFER
#include <dev/framebuffer.h>
#endif

page_dir_entry_t* _kernel_vm = NULL;
uint32_t _mmio_base = 0;

/*Copy interrupt talbe to phymen address 0x00000000.
	Virtual address #INTERRUPT_VECTOR_BASE(0xFFFF0000 for ARM) must mapped to phymen 0x00000000.
ref: set_kernel_vm(page_dir_entry_t* vm)
 */
static void __attribute__((optimize("O0"))) copy_interrupt_table(void) {
	extern uint32_t  interrupt_table_start, interrupt_table_end;
	uint32_t *vsrc = &interrupt_table_start;
	//uint32_t *vdst = (uint32_t*)INTERRUPT_VECTOR_BASE;
	uint32_t *vdst = (uint32_t*)0x0;
	while(vsrc < &interrupt_table_end) {
		*vdst++ = *vsrc++;
	}
}

//static void __attribute__((optimize("O0"))) set_kernel_init_vm(page_dir_entry_t* vm) {
static void set_kernel_init_vm(page_dir_entry_t* vm) {
	memset(vm, 0, PAGE_DIR_SIZE);

	//map interrupt vector to high(virtual) mem
	map_pages(vm, 0, 0, PAGE_SIZE, AP_RW_D, 0);
	map_pages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D, 0);

	//map kernel image, page dir, kernel malloc mem
	map_pages(vm, KERNEL_BASE+PAGE_SIZE, PAGE_SIZE, V2P(ALLOCATABLE_PAGE_DIR_END), AP_RW_D, 0);

	//map MMIO to high(virtual) mem.
	hw_info_t* hw_info = get_hw_info();
	map_pages(vm, MMIO_BASE, hw_info->phy_mmio_base, hw_info->phy_mmio_base + hw_info->mmio_size, AP_RW_D, 1);
	arch_vm(vm);
}

//void __attribute__((optimize("O0"))) set_kernel_vm(page_dir_entry_t* vm) {
void set_kernel_vm(page_dir_entry_t* vm) {
	set_kernel_init_vm(vm);
	map_pages(vm, 
		ALLOCATABLE_MEMORY_START, 
		V2P(ALLOCATABLE_MEMORY_START),
		get_hw_info()->phy_mem_size,
		AP_RW_D, 0);
}

//static void __attribute__((optimize("O0"))) init_kernel_vm(void) {
static void init_kernel_vm(void) {
	int32_t i;
	for(i=0; i<RAM_HOLE_MAX; i++)
		_ram_holes[i].base = _ram_holes[i].end = 0;

	_kernel_vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
	//get kalloc ready just for kernel page tables.
	kalloc_init(KERNEL_PAGE_DIR_BASE+PAGE_DIR_SIZE, KERNEL_PAGE_DIR_END, false); 
	set_kernel_init_vm(_kernel_vm);

	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	set_translation_table_base(V2P((uint32_t)_kernel_vm));
}

static void init_allocable_mem(void) {
	kalloc_init(ALLOCATABLE_PAGE_DIR_BASE, ALLOCATABLE_PAGE_DIR_END, false);
	map_pages(_kernel_vm,
		ALLOCATABLE_MEMORY_START,
		V2P(ALLOCATABLE_MEMORY_START),
		get_hw_info()->phy_mem_size,
		AP_RW_D, 0);

	kalloc_init(ALLOCATABLE_MEMORY_START, P2V(get_hw_info()->phy_mem_size)-24*MB, true);
}

static void dev_init(void) {
#ifdef SDC
  printf("    %16s ", "mmc_sd");
  if(sd_init() == 0) {
    printf("[OK]\n\n");
  }
  else
    printf("[Failed!]\n");
#endif
}

#ifdef SDC
int32_t load_init_sdc(void);
int32_t load_init_proc(void) {
	get_hw_info()->kfs = 0;
	return load_init_sdc();
}
#else
int32_t load_init_kfs(void);
int32_t load_init_proc(void) {
	get_hw_info()->kfs = 1;
	return load_init_kfs();
}
#endif

//void  __attribute__((optimize("O0"))) _kernel_entry_c(context_t* ctx) {
void _kernel_entry_c(context_t* ctx) {
	(void)ctx;
	//clear bss
	memset(_bss_start, 0, (uint32_t)_bss_end - (uint32_t)_bss_start);
	copy_interrupt_table();
	hw_info_init();
	_mmio_base = MMIO_BASE;

	hw_optimise();

	init_kernel_vm();  
	km_init();

	uart_dev_init();
	const char* msg = "\n\n"
			"===Ewok micro-kernel===\n\n"
			"kernel: mmu inited\n"
			"kernel: uart inited\n";
	uart_write(msg, strlen(msg));

	kev_init();

#ifdef DISPLAY
	kconsole_init();
#endif

#ifdef FRAMEBUFFER
	printf("kernel: framebuffer initing\n");
	if(fb_dev_init(1280, 720, 32) == 0) {
	//if(fb_dev_init(640, 480, 16) == 0) {
		fbinfo_t* info = fb_get_info();
		printf("    [OK] : %dx%d %dbits, addr: 0x%X, size:%d\n", 
				info->width, info->height, info->depth,
				info->pointer, info->size);
		memset((void*)info->pointer, 0, info->size);
		kconsole_setup();
	}
	else {
		printf("  [Failed!]\n");
	}
#endif

	printf("kernel: kmalloc initing  [ok] : %dMB\n", div_u32(KMALLOC_END-KMALLOC_BASE, 1*MB));
	init_allocable_mem(); //init the rest allocable memory VM
	printf("kernel: init allocable memory: %dMB\n", div_u32(get_free_mem_size(), 1*MB));

	printf("kernel: devices initing.\n");
	dev_init();
	

	shm_init();
	printf("kernel: share memory inited.\n");

	procs_init();
	printf("kernel: processes inited.\n");

	irq_init();
	printf("kernel: irq inited\n");

	printf("kernel: loading init");
	if(load_init_proc() != 0) 
		printf(" [failed!]\n");
	else
		printf(" [ok]\n");
	
	printf("kernel: start timer.\n");
	timer_set_interval(0, 0x200); 

	while(1) {
		__asm__("MOV r0, #0; MCR p15,0,R0,c7,c0,4"); // CPU enter WFI state
	}

#ifdef DISPLAY
	kconsole_close();
#endif
}
