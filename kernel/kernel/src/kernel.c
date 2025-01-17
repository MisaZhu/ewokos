#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/shm.h>
#include <mm/dma.h>
#include <kstring.h>
#include <kernel/core.h>
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
#include <stddef.h>
#include <sysinfo.h>
#include <kernel/kconsole.h>
#include <kernel/semaphore.h>

page_dir_entry_t* _kernel_vm = NULL;
vsyscall_info_t* _kernel_vsyscall_info = NULL;

/*Copy interrupt talbe to phymen address 0x00000000.
	Virtual address #INTERRUPT_VECTOR_BASE(0xFFFF0000 for ARM) must mapped to phymen 0x00000000.
ref: set_vm(page_dir_entry_t* vm)
 */
static void __attribute__((optimize("O0"))) copy_interrupt_table(void) {
	extern uint32_t  interrupt_table_start, interrupt_table_end;
	uint32_t *vsrc = &interrupt_table_start;
	//uint32_t *vdst = (uint32_t*)INTERRUPT_VECTOR_BASE;
	uint32_t *vdst = (uint32_t*)(_sys_info.vector_base);
	while(vsrc < &interrupt_table_end) {
		*vdst++ = *vsrc++;
	}
}

static void set_kernel_vm(page_dir_entry_t* vm) {
	memset(vm, 0, PAGE_DIR_SIZE);
	flush_dcache();

	//map interrupt vector to high(virtual) mem
	map_pages_size(vm, INTERRUPT_VECTOR_BASE, _sys_info.vector_base, PAGE_SIZE, AP_RW_D, PTE_ATTR_WRBACK);
	//map kernel image
	map_pages(vm, KERNEL_BASE, _sys_info.phy_offset, V2P(KERNEL_IMAGE_END), AP_RW_D, PTE_ATTR_WRBACK_ALLOCATE);
	//map kernel page dir
	map_pages(vm, KERNEL_PAGE_DIR_BASE, V2P(KERNEL_PAGE_DIR_BASE), V2P(KERNEL_PAGE_DIR_END), AP_RW_D, PTE_ATTR_WRBACK);

	//map kernel sys_state memory
	map_pages(vm, KERNEL_VSYSCALL_INFO_BASE, V2P(KERNEL_VSYSCALL_INFO_BASE), V2P(KERNEL_VSYSCALL_INFO_END), AP_RW_R, PTE_ATTR_DEV);
	_kernel_vsyscall_info = (vsyscall_info_t*) KERNEL_VSYSCALL_INFO_BASE;

	//map kernel malloc memory
	map_pages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_END), AP_RW_D, PTE_ATTR_WRBACK);
	//map allocatable memory page dir
	map_pages(vm, ALLOCABLE_PAGE_DIR_BASE, V2P(ALLOCABLE_PAGE_DIR_BASE), V2P(ALLOCABLE_PAGE_DIR_END), AP_RW_D, PTE_ATTR_WRBACK);
	//map MMIO to high(virtual) mem.
	map_pages_size(vm, _sys_info.mmio.v_base, _sys_info.mmio.phy_base, _sys_info.mmio.size, AP_RW_D, PTE_ATTR_DEV);

	arch_vm(vm);
}

static void reset_kernel_vm(void) {
	page_dir_entry_t* vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
	//map kernel malloc memory
	map_pages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_END), AP_RW_D, PTE_ATTR_WRBACK);
	//map allocatable memory page dir
	map_pages(vm, ALLOCABLE_PAGE_DIR_BASE, V2P(ALLOCABLE_PAGE_DIR_BASE), V2P(ALLOCABLE_PAGE_DIR_END), AP_RW_D, PTE_ATTR_WRBACK);
	//map MMIO to high(virtual) mem.
	flush_tlb();
}


static void map_allocable_pages(page_dir_entry_t* vm) {
	map_pages(vm,
			P2V(_allocable_phy_mem_base),
			_allocable_phy_mem_base,
			_allocable_phy_mem_top,
			AP_RW_D, PTE_ATTR_WRBACK);
	flush_tlb();
}

void set_vm(page_dir_entry_t* vm) {
	set_kernel_vm(vm);
	map_allocable_pages(vm);
}

static void init_kernel_vm(void) {
	_pages_ref.max = 0;
	_kernel_vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
	//get kalloc(4k memblocks) ready just for kernel page tables.
	kalloc_init(KERNEL_PAGE_DIR_BASE+PAGE_DIR_SIZE, KERNEL_PAGE_DIR_END); 

	//switch to two-levels 4k page_size type paging
	set_kernel_vm(_kernel_vm);
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	set_translation_table_base(V2P((uint32_t)_kernel_vm));
}

static void init_allocable_mem(void) {
	//printf("kernel: kalloc init for allocable page dir\n");
	kalloc_init(ALLOCABLE_PAGE_DIR_BASE, ALLOCABLE_PAGE_DIR_END); 
	//printf("kernel: mapping allocable pages\n");
	map_allocable_pages(_kernel_vm);
	
	_pages_ref.max = kalloc_init(P2V(_allocable_phy_mem_base), P2V(_allocable_phy_mem_top));
	_pages_ref.refs = kmalloc(_pages_ref.max * sizeof(page_ref_t));	
	_pages_ref.phy_base = _allocable_phy_mem_base;
	memset(_pages_ref.refs, 0, _pages_ref.max * sizeof(page_ref_t));
}

#ifdef KERNEL_SMP
void __attribute__((optimize("O0"))) _slave_kernel_entry_c(void) {
	set_translation_table_base(V2P((uint32_t)_kernel_vm));

	uint32_t cid = get_core_id();
	cpu_core_ready(cid);
	_cpu_cores[cid].actived = true;

	halt();
}
#endif

static void logo(void) {
	printf(
			"---------------------------------------------------\n"
			" ______           ______  _    _   ______  ______ \n"
			"(  ___ \\|\\     /|(  __  )| \\  / \\ (  __  )(  ___ \\\n"
			"| (__   | | _ | || |  | || (_/  / | |  | || (____\n"
			"|  __)  | |( )| || |  | ||  _  (  | |  | |(____  )\n"
			"| (___  | || || || |__| || ( \\  \\ | |__| |  ___) |\n"
			"(______/(_______)(______)|_/  \\_/ (______)\\______)\n\n");
}

static void show_config(void) {
	printf("\n"
		  "    machine              %s\n" 
		  "    arch                 %s\n"
		  "    cores                %d\n"
		  "    kernel_timer_freq    %d\n"
		  "    mem_offset           0x%x\n"
		  "    mem_size             %d MB\n"
		  "    kmalloc size         %d MB\n"
		  "    mmio_base            Phy:0x%x, V: 0x%x\n"
		  "    max proc num         %d\n"
		  "    max task total       %d\n"
		  "    max task per proc    %d\n\n",
			_sys_info.machine,
			_sys_info.arch,
			_kernel_config.cores,
			_kernel_config.timer_freq,
			_sys_info.phy_offset,
			_sys_info.phy_mem_size/1024/1024,
			(KMALLOC_END-KMALLOC_BASE) / (1*MB),
			_sys_info.mmio.phy_base, _sys_info.mmio.v_base,
			_kernel_config.max_proc_num,
			_kernel_config.max_task_num,
			_kernel_config.max_task_per_proc);
}

int32_t load_init_proc(void);
void _kernel_entry_c(void) {
	__irq_disable();
	//clear bss
	memset(_bss_start, 0, (uint32_t)_bss_end - (uint32_t)_bss_start);
	sys_info_init();

	copy_interrupt_table();

	init_kernel_vm();  
	uart_dev_init(19200);
	kout  ("\n=== ewokos booting ===\n\n");
	kout  ("kernel: init kernel malloc     ... ");
	kmalloc_init(); //init kmalloc with min size for just early stage kernel load
	kout  ("[OK]\n");

	kout  ("kernel: init kernel event      ... ");
	kev_init();
	kout  ("[OK]\n");

	kout  ("kernel: init sd                ... ");
	sd_init();
	kout  ("[OK]\n");

	kout  ("kernel: load kernel config     ... ");
	load_kernel_config();
	kout  ("[OK]\n");

	uart_dev_init(_kernel_config.uart_baud);

	show_config();

	kout  ("kernel: remapping kernel mem   ... ");
	reset_kernel_vm();
	kmalloc_init(); //init kmalloc again with config info;
	kout  ("[OK]\n");

#ifdef KCONSOLE
	kout  ("kernel: init framebuffer       ... ");
	kconsole_init();
	kout  ("[OK]\n");
#endif
	logo();

	printf("kernel: init allocable memory  ... ");
	init_allocable_mem(); //init the rest allocable memory VM
	printf("[ok] (%d MB)\n", (get_free_mem_size() / (1*MB)));

	printf("kernel: init DMA               ... ");
	dma_init();
	printf("[OK]\n");

	printf("kernel: init semaphore         ... ");
	semaphore_init();
	printf("[ok]\n");

	printf("kernel: init irq               ... ");
	irq_init();
	printf("[ok]\n");

	printf("kernel: init share memory      ... ");
	shm_init();
	printf("[ok]\n");

	printf("kernel: init processes table   ... ");
	if(procs_init() != 0)
		halt();
	printf("[ok] (%d)\n", _kernel_config.max_proc_num);

	printf("kernel: loading init process   ... ");
	if(load_init_proc() != 0)  {
		printf("[failed!]\n");
		halt();
	}
	printf("[ok]\n");

	kfork_core_halt(0);
#ifdef KERNEL_SMP
	kernel_lock_init();

	for(uint32_t i=1; i<_sys_info.cores; i++) {
		_cpu_cores[i].actived = false;
		printf("kernel: start core %d           ... ", i);
		kfork_core_halt(i);
		start_core(i);
		while(!_cpu_cores[i].actived)
			_delay_msec(10);
		printf("[ok]\n");
	}
#endif

	printf("kernel: set timer              ... ");
	timer_set_interval(0, _kernel_config.timer_freq); 
	printf("[ok]\n");
	printf("kernel: start init process     ...\n"
		   "---------------------------------------------------\n");
	__irq_enable();
	halt();

	kfree4k(_kernel_vsyscall_info);
}
