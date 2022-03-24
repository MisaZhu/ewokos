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
#include <dev/dev.h>
#include <dev/uart.h>
#include <basic_math.h>
#include <stddef.h>

page_dir_entry_t* _kernel_vm = NULL;

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

static void set_kernel_init_vm(page_dir_entry_t* vm) {
	memset(vm, 0, PAGE_DIR_SIZE);

	//map interrupt vector to high(virtual) mem
	map_pages(vm, 0, 0, PAGE_SIZE, AP_RW_D, 0);
	map_pages(vm, INTERRUPT_VECTOR_BASE, 0, PAGE_SIZE, AP_RW_D, 0);

	//map kernel image, page dir, kernel malloc mem
	map_pages(vm, KERNEL_BASE+PAGE_SIZE, PAGE_SIZE, V2P(ALLOCATABLE_PAGE_DIR_END), AP_RW_D, 0);

	//map MMIO to high(virtual) mem.
	map_pages(vm, _sys_info.mmio.phy_base, _sys_info.mmio.phy_base, _sys_info.mmio.phy_base + _sys_info.mmio.size, AP_RW_D, 1);
	map_pages(vm, MMIO_BASE, _sys_info.mmio.phy_base, _sys_info.mmio.phy_base + _sys_info.mmio.size, AP_RW_D, 1);
	arch_vm(vm);
	flush_tlb();
}

void set_kernel_vm(page_dir_entry_t* vm) {
	set_kernel_init_vm(vm);
	map_pages(vm, 
		ALLOCATABLE_MEMORY_START, 
		V2P(ALLOCATABLE_MEMORY_START),
		_sys_info.phy_mem_size,
		AP_RW_D, 0);
	flush_tlb();
}

static void init_kernel_vm(void) {
	_kernel_vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
	//get kalloc ready just for kernel page tables.
	kalloc_init(KERNEL_PAGE_DIR_BASE+PAGE_DIR_SIZE, KERNEL_PAGE_DIR_END); 
	set_kernel_init_vm(_kernel_vm);

	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	set_translation_table_base(V2P((uint32_t)_kernel_vm));
}

static void init_allocable_mem(void) {
	printf("kernel: kalloc init for allocatable page dir\n");
	kalloc_init(ALLOCATABLE_PAGE_DIR_BASE, ALLOCATABLE_PAGE_DIR_END); 
	printf("kernel: mapping allocatable pages\n");
	map_pages(_kernel_vm,
			P2V(_allocatable_mem_base),
			_allocatable_mem_base,
			_allocatable_mem_top,
			AP_RW_D, 0);
	flush_tlb();
	printf("kernel: kalloc init for all allocatable pages\n");
	_pages_ref.max = kalloc_init(P2V(_allocatable_mem_base), P2V(_allocatable_mem_top));
	_pages_ref.refs = kmalloc(_pages_ref.max * sizeof(page_ref_t));	
	_pages_ref.phy_base = _allocatable_mem_base;
	memset(_pages_ref.refs, 0, _pages_ref.max * sizeof(page_ref_t));
}

#ifdef KERNEL_SMP
static uint32_t _started_cores = 0;
void __attribute__((optimize("O0"))) _slave_kernel_entry_c(void) {
	while(1) {
		if(multi_cores_ready() == 0)
			break;
	}
	set_translation_table_base(V2P((uint32_t)_kernel_vm));
	kernel_lock();
	//printf("kernel: start core %d\n", get_core_id());
	_started_cores++;
	kernel_unlock();
	halt();
	//while(1);
}
#endif

int32_t load_init_proc(void);
void _kernel_entry_c(void) {
	__irq_disable();
	//clear bss
	memset(_bss_start, 0, (uint32_t)_bss_end - (uint32_t)_bss_start);
	copy_interrupt_table();
	_pages_ref.max = 0;

	sys_info_init();
	init_kernel_vm();  
	km_init();
	kev_init();

	enable_vmmio_base();
	dev_init();

	printf(
			" ______           ______  _    _   ______  ______ \n"
			"(  ___ \\|\\     /|(  __  )| \\  / \\ (  __  )(  ___ \\\n"
			"| (__   | | _ | || |  | || (_/  / | |  | || (____\n"
			"|  __)  | |( )| || |  | ||  _  (  | |  | |(____  )\n"
			"| (___  | || || || |__| || ( \\  \\ | |__| |  ___) |\n"
			"(______/(_______)(______)|_/  \\_/ (______)\\______)\n\n");
                                                      
	printf("kernel: kmalloc initing  [ok] : %dMB\n", div_u32(KMALLOC_END-KMALLOC_BASE, 1*MB));
	init_allocable_mem(); //init the rest allocable memory VM
	printf("kernel: init allocable memory: %dMB, %d pages\n", div_u32(get_free_mem_size(), 1*MB), _pages_ref.max);

#ifdef KERNEL_SMP
	_started_cores = 1;
	uint32_t cores = get_cpu_cores();
	start_multi_cores(cores);
#endif

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
	printf("  [ok]\n");

#ifdef KERNEL_SMP
	for(uint32_t i=0; i<cores; i++) {
		proc_t* p = kfork_core_halt(i);
		_cpu_cores[i].halt_pid = p->info.pid;
	}

	while(_started_cores < cores) {
		printf("kernel: started: %d\n", _started_cores);
	}
	printf("kernel: start SMP (%d cores)\n", cores);
#endif

	printf("kernel: set timer.\n");
	timer_set_interval(0, 512); 

	printf("kernel: enable irq.\n");
	__irq_enable();

	halt();
}

