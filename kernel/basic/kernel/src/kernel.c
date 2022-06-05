#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/shm.h>
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
#include <basic_math.h>
#include <stddef.h>
#include <kernel/kconsole.h>
#include <gic.h>

page_dir_entry_t* _kernel_vm = NULL;

/*Copy interrupt talbe to phymen address 0x00000000.
	Virtual address #INTERRUPT_VECTOR_BASE(0xFFFF0000 for ARM) must mapped to phymen 0x00000000.
ref: set_kernel_vm(page_dir_entry_t* vm)
 */
static void __attribute__((optimize("O0"))) copy_interrupt_table(void) {
	extern uint32_t  interrupt_table_start, interrupt_table_end;
	uint32_t *vsrc = &interrupt_table_start;
	//uint32_t *vdst = (uint32_t*)INTERRUPT_VECTOR_BASE;
	uint32_t *vdst = (uint32_t*)_sys_info.phy_offset;
	while(vsrc < &interrupt_table_end) {
		*vdst++ = *vsrc++;
	}
}

static void set_kernel_init_vm(page_dir_entry_t* vm) {
	memset(vm, 0, PAGE_DIR_SIZE);
	flush_dcache();

	//map interrupt vector to high(virtual) mem
	map_pages_size(vm, INTERRUPT_VECTOR_BASE, _sys_info.phy_offset, PAGE_SIZE, AP_RW_D, PTE_ATTR_WRBACK);
	//map kernel image
	map_pages(vm, KERNEL_BASE, _sys_info.phy_offset, V2P(KERNEL_IMAGE_END), AP_RW_D, PTE_ATTR_WRBACK_ALLOCE);
	//map kernel page dir
	map_pages(vm, KERNEL_PAGE_DIR_BASE, V2P(KERNEL_PAGE_DIR_BASE), V2P(ALLOCATABLE_PAGE_DIR_END), AP_RW_D, PTE_ATTR_WRBACK);
	//map kernel malloc memory
	map_pages(vm, KMALLOC_BASE, V2P(KMALLOC_BASE), V2P(KMALLOC_END), AP_RW_D, PTE_ATTR_WRBACK);
	//map allocatable memory page dir
	map_pages(vm, ALLOCATABLE_PAGE_DIR_BASE, V2P(ALLOCATABLE_PAGE_DIR_BASE), V2P(ALLOCATABLE_PAGE_DIR_END), AP_RW_D, PTE_ATTR_WRBACK);
	//map MMIO to high(virtual) mem.
	map_pages_size(vm, _sys_info.mmio.v_base, _sys_info.mmio.phy_base, _sys_info.mmio.size, AP_RW_D, PTE_ATTR_DEV);

	arch_vm(vm);
}

static void map_allocatable_pages(page_dir_entry_t* vm) {
	map_pages(vm,
			P2V(_allocatable_phy_mem_base),
			_allocatable_phy_mem_base,
			_allocatable_phy_mem_top,
			AP_RW_D, PTE_ATTR_WRBACK);
	flush_tlb();
}

void set_kernel_vm(page_dir_entry_t* vm) {
	set_kernel_init_vm(vm);
	map_allocatable_pages(vm);
}

static void init_kernel_vm(void) {
	_pages_ref.max = 0;
	_kernel_vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
	//get kalloc(4k memblocks) ready just for kernel page tables.
	kalloc_init(KERNEL_PAGE_DIR_BASE+PAGE_DIR_SIZE, KERNEL_PAGE_DIR_END); 

	//switch to two-levels 4k page_size type paging
	set_kernel_init_vm(_kernel_vm);
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	set_translation_table_base(V2P((uint32_t)_kernel_vm));
}

static void init_allocable_mem(void) {
	printf("kernel: kalloc init for allocatable page dir\n");
	kalloc_init(ALLOCATABLE_PAGE_DIR_BASE, ALLOCATABLE_PAGE_DIR_END); 
	printf("kernel: mapping allocatable pages\n");
	map_allocatable_pages(_kernel_vm);
	
	printf("kernel: kalloc init for all allocatable pages\n");
	_pages_ref.max = kalloc_init(P2V(_allocatable_phy_mem_base), P2V(_allocatable_phy_mem_top));
	_pages_ref.refs = kmalloc(_pages_ref.max * sizeof(page_ref_t));	
	_pages_ref.phy_base = _allocatable_phy_mem_base;
	memset(_pages_ref.refs, 0, _pages_ref.max * sizeof(page_ref_t));
}

#ifdef KERNEL_SMP
void __attribute__((optimize("O0"))) _slave_kernel_entry_c(void) {
	/*while(1) {
		if(multi_cores_ready() == 0)
			break;
	}
	*/
	set_translation_table_base(V2P((uint32_t)_kernel_vm));
	printf("[ok]\n");

	uint32_t cid = get_core_id();
	cpu_core_ready(cid);
	_cpu_cores[cid].actived = true;

	halt();
}
#endif

static void welcome(void) {
	printf( "\n"
			"---------------------------------------------------\n"
			" ______           ______  _    _   ______  ______ \n"
			"(  ___ \\|\\     /|(  __  )| \\  / \\ (  __  )(  ___ \\\n"
			"| (__   | | _ | || |  | || (_/  / | |  | || (____\n"
			"|  __)  | |( )| || |  | ||  _  (  | |  | |(____  )\n"
			"| (___  | || || || |__| || ( \\  \\ | |__| |  ___) |\n"
			"(______/(_______)(______)|_/  \\_/ (______)\\______)\n\n");
                                                      
	printf("machine        %s\n" 
		   "mem_size       %d MB\n"
		   "mem_offset     0x%x\n"
		   "mmio_base      0x%x\n"
		   "---------------------------------------------------\n\n",
			_sys_info.machine,
			_sys_info.phy_mem_size/1024/1024,
			_sys_info.phy_offset,
			_sys_info.mmio.phy_base);
}

int32_t load_init_proc(void);
void _kernel_entry_c(void) {
	__irq_disable();
	//clear bss
	memset(_bss_start, 0, (uint32_t)_bss_end - (uint32_t)_bss_start);
	sys_info_init();

	copy_interrupt_table();

	init_kernel_vm();  
	kmalloc_init();

	uart_dev_init();
	kev_init();

#ifdef KCONSOLE
	kconsole_init();
#endif
	welcome();

	printf("kernel: kmalloc initing  [ok] : %dMB\n", (KMALLOC_END-KMALLOC_BASE) / (1*MB));
	init_allocable_mem(); //init the rest allocable memory VM
	printf("kernel: init allocable memory: %dMB, %d pages\n", (get_free_mem_size() / (1*MB)), _pages_ref.max);

	irq_init();
	printf("kernel: irq inited\n");

	shm_init();
	printf("kernel: share memory inited.\n");

	procs_init();
	printf("kernel: processes inited.\n");

	printf("kernel: loading init process\n");
	if(load_init_proc() != 0)  {
		printf("  [failed!]\n");
		halt();
	}

	kfork_core_halt(0);
#ifdef KERNEL_SMP
	kernel_lock_init();

	for(uint32_t i=1; i<_sys_info.cores; i++) {
		_cpu_cores[i].actived = false;
		printf("kernel: start core %d ... ", i);
		kfork_core_halt(i);
		start_core(i);
		while(!_cpu_cores[i].actived)
			_delay_msec(10);
	}
#endif

	printf("kernel: set timer.\n");
	timer_set_interval(0, 512); 

	printf("kernel: enable irq and start init...\n");

#ifdef KCONSOLE
	kconsole_close();
#endif
	__irq_enable();
	halt();
}
