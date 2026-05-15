#include <mm/mmu.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/kmalloc_vm.h>
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
#include <dev/sd.h>
#include <stddef.h>
#include <sysinfo.h>
#include <kernel/semaphore.h>



kernel_info_t _kernel_info;

static void boot_mark(char c) {
#ifdef __x86_64__
	uint8_t ready;

	do {
		__asm__ volatile("inb %1, %0" : "=a"(ready) : "Nd"((uint16_t)0x3FD));
	} while ((ready & 0x20) == 0);

	__asm__ volatile("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
#else
	(void)c;
#endif
}

/*Copy interrupt talbe to phymen address 0x00000000.
	Virtual address #INTERRUPT_VECTOR_BASE(0xFFFF0000 for ARM) must mapped to phymen 0x00000000.
ref: set_vm(page_dir_entry_t* vm)
 */
static void __attribute__((optimize("O0"))) copy_interrupt_table(void) {
	uint32_t *vsrc = &interrupt_table_start;
	//uint32_t *vdst = (uint32_t*)INTERRUPT_VECTOR_BASE;
	uint32_t *vdst = (uint32_t*)(_sys_info.vector_base);
	if(vsrc == vdst)
		return;
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
	map_pages(vm, KERNEL_VSYSCALL_INFO_BASE, V2P(KERNEL_VSYSCALL_INFO_BASE), V2P(KERNEL_VSYSCALL_INFO_END), AP_RW_RW, PTE_ATTR_DEV);
	_kernel_info.vsyscall_info = (vsyscall_info_t*) KERNEL_VSYSCALL_INFO_BASE;

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
	//map kernel dma memory
	map_pages_size(vm, _sys_info.sys_dma.phy_base, _sys_info.sys_dma.phy_base, _sys_info.sys_dma.size, AP_RW_D, PTE_ATTR_WRBACK);
	map_pages(vm,
			P2V(_sys_info.allocable_phy_mem_base),
			_sys_info.allocable_phy_mem_base,
			_sys_info.allocable_phy_mem_top,
			AP_RW_D, PTE_ATTR_WRBACK);
	flush_tlb();
}

#ifdef __x86_64__
static inline page_table_entry_t* entry_to_table_local(page_table_entry_t* entry) {
	return (page_table_entry_t*)P2V((ewokos_addr_t)(entry->Address << 12));
}

static void clone_kernel_vm(page_dir_entry_t* vm) {
	page_table_entry_t* kernel_pdpt;
	page_table_entry_t* proc_pdpt;

	memset(vm, 0, PAGE_DIR_SIZE);
	flush_dcache();

	proc_pdpt = kalloc4k();
	if (proc_pdpt == NULL) {
		return;
	}
	memset(proc_pdpt, 0, PAGE_TABLE_SIZE);

	vm[0].value = 0;
	vm[0].present = 1;
	vm[0].rw = 1;
	vm[0].us = 1;
	vm[0].Address = (uint64_t)V2P(proc_pdpt) >> 12;

	kernel_pdpt = entry_to_table_local(&_kernel_info.kernel_vm[0]);
	proc_pdpt[2] = kernel_pdpt[2];
	proc_pdpt[3] = kernel_pdpt[3];

	// Keep the low DMA identity window available while leaving the rest of
	// the user half private for per-process mappings.
	map_pages_size(vm, _sys_info.sys_dma.phy_base, _sys_info.sys_dma.phy_base,
			_sys_info.sys_dma.size, AP_RW_D, PTE_ATTR_WRBACK);
	flush_tlb();
}
#endif

void set_vm(page_dir_entry_t* vm) {
#ifdef __x86_64__
	clone_kernel_vm(vm);
#else
	set_kernel_vm(vm);
	map_allocable_pages(vm);
#endif
}

static void init_kernel_vm(void) {
	_pages_ref.max = 0;
	_kernel_info.kernel_vm = (page_dir_entry_t*)KERNEL_PAGE_DIR_BASE;
	//get kalloc(4k memblocks) ready just for kernel page tables.
	kalloc_reset();
	kalloc_append(KERNEL_PAGE_DIR_BASE+PAGE_DIR_SIZE, KERNEL_PAGE_DIR_END); 

	//switch to two-levels 4k page_size type paging
	set_kernel_vm(_kernel_info.kernel_vm);
	//Use physical address of kernel virtual memory as the new virtual memory page dir table base.
	set_translation_table_base(V2P((uint32_t)_kernel_info.kernel_vm));
}

static void init_allocable_mem(void) {
	//printf("kernel: kalloc init for allocable page dir\n");
	kalloc_append(ALLOCABLE_PAGE_DIR_BASE, ALLOCABLE_PAGE_DIR_END); 
	//printf("kernel: mapping allocable pages\n");
	map_allocable_pages(_kernel_info.kernel_vm);
	
	//_pages_ref.max = kalloc_append(P2V(_sys_info.allocable_phy_mem_base), P2V(_sys_info.allocable_phy_mem_top));
	kalloc_arch();
	_pages_ref.max = (_sys_info.allocable_phy_mem_top - _sys_info.allocable_phy_mem_base) / PAGE_SIZE;
	_pages_ref.refs = kmalloc(_pages_ref.max * sizeof(page_ref_t));	
	_pages_ref.phy_base = _sys_info.allocable_phy_mem_base;
	memset(_pages_ref.refs, 0, _pages_ref.max * sizeof(page_ref_t));
}

#ifdef KERNEL_SMP
void __attribute__((optimize("O0"))) _slave_kernel_entry_c(void) {
	set_translation_table_base(V2P((uint32_t)_kernel_info.kernel_vm));

	uint32_t cid = get_core_id();
	cpu_core_ready(cid);
	_cpu_cores[cid].actived = true;

	halt();
}
#endif

static void logo(void) {
	printf(
			"-----------------------------------------------------\n"
			" ______           ______  _    _   ______  ______ \n"
			"(  ___ \\|\\     /|(  __  )| \\  / \\ (  __  )(  ___ \\\n"
			"| (__   | | _ | || |  | || (_/  / | |  | || (____\n"
			"|  __)  | |( )| || |  | ||  _  (  | |  | |(____  )\n"
			"| (___  | || || || |__| || ( \\  \\ | |__| |  ___) |\n"
			"(______/(_______)(______)|_/  \\_/ (______)\\______)\n");
}

static void show_config(void) {
	printf("\n"
		  "  machine              %s\n" 
		  "  arch                 %s\n"
		  "  cores                %d\n"
		  "  kernel_timer_freq    %d\n"
		  "  mem_offset           0x%08x\n"
		  "  phy mem size         %d MB\n"
		  "  usable mem size      %d MB\n"
		  "  mmio_base            Phy:0x%08x V:0x%08x (%d MB)\n"
		  "  kernel image         Phy:0x%08x ~ 0x%08x (%d KB)\n"
		  "  vsyscall info        Phy:0x%08x ~ 0x%08x (%d KB)\n"
		  "  kernel page dir      Phy:0x%08x ~ 0x%08x (%d KB)\n"
		  "  allocable page dir   Phy:0x%08x ~ 0x%08x (%d MB)\n"
		  "  kmalloc              Phy:0x%08x ~ 0x%08x (%d MB)\n"
		  "  sys_dma_base         Phy:0x%08x ~ 0x%08x (%d MB)\n"
		  "  allocable mem info   Phy:0x%08x ~ 0x%08x (%d MB)\n"
		  "  max proc num         %d\n"
		  "  max task total       %d\n"
		  "  max task per proc    %d\n"
		  "-----------------------------------------------------\n",
			_sys_info.machine,
			_sys_info.arch,
			_kernel_config.cores,
			_kernel_config.timer_freq,
			_sys_info.phy_offset,
			_sys_info.total_phy_mem_size/(1*MB),
			_sys_info.total_usable_mem_size / (1*MB),
			_sys_info.mmio.phy_base, _sys_info.mmio.v_base, _sys_info.mmio.size/(1*MB),
			V2P(_kernel_start), V2P(_kernel_end), (_kernel_end - _kernel_start) / (1*KB),
			V2P(KERNEL_VSYSCALL_INFO_BASE), V2P(KERNEL_VSYSCALL_INFO_END), KERNEL_VSYSCALL_INFO_SIZE / (1*KB),
			V2P(KERNEL_PAGE_DIR_BASE), V2P(KERNEL_PAGE_DIR_END), KERNEL_PAGE_DIR_SIZE / (1*KB),
			V2P(ALLOCABLE_PAGE_DIR_BASE), V2P(ALLOCABLE_PAGE_DIR_END), ALLOCABLE_PAGE_DIR_SIZE / (1*MB),
			V2P(KMALLOC_BASE), V2P(KMALLOC_END), _sys_info.kmalloc_size / (1*MB),
			_sys_info.sys_dma.phy_base, _sys_info.sys_dma.phy_base+_sys_info.sys_dma.size, _sys_info.sys_dma.size/(1*MB),
			_sys_info.allocable_phy_mem_base, _sys_info.allocable_phy_mem_top, get_free_mem_size() / (1*MB),
			_kernel_config.max_proc_num,
			_kernel_config.max_task_num,
			_kernel_config.max_task_per_proc);
}

int32_t load_init_proc(void);
void _kernel_entry_c(void) {
#ifdef __x86_64__
	__asm__ volatile("cli");
#endif
	boot_mark('N');
	//clear bss
	memset(_bss_start, 0, (size_t)(_bss_end - _bss_start));
	sys_info_init();
	boot_mark('O');

	copy_interrupt_table();
	boot_mark('P');

	init_kernel_vm();  
	boot_mark('Q');
	uart_dev_init(19200);
	boot_mark('R');
	kout_str("\n=== ewokos booting ===\n\n");
	boot_mark('S');
	kout_str("kernel: init kernel malloc     ... ");
	kmalloc_init(); //init kmalloc with min size for just early stage kernel load
	kout_str("[OK]\n");

	kout_str("kernel: init sd                ... ");
	sd_init();
	kout_str("[OK]\n");

	kout_str("kernel: load kernel config     ... ");
	load_kernel_config();
	boot_mark('T');
	kout_str("[OK]\n");

	boot_mark('U');
	uart_dev_init(_kernel_config.uart_baud);
	boot_mark('V');

	boot_mark('W');
	kout_str("kernel: remapping kernel mem   ... ");
	boot_mark('X');
	reset_kernel_vm();
	boot_mark('Y');
	kmalloc_init(); //init kmalloc again with config info;
	boot_mark('Z');
	kmalloc_vm_init(); //init kmalloc extra;
	boot_mark('a');
	kout_str("[OK]\n");

	//printf("kernel: init allocable memory  ... ");
	boot_mark('b');
	init_allocable_mem(); //init the rest allocable memory VM
	boot_mark('c');
	//printf("[ok] (%d MB)\n", (get_free_mem_size() / (1*MB)));

	boot_mark('d');
	logo();
	boot_mark('e');
	show_config();

	kout_str("kernel: init kernel event      ... ");
	boot_mark('f');
	kev_init();
	boot_mark('g');
	kout_str("[OK]\n");

	//printf("kernel: init DMA               ... ");
	boot_mark('h');
	dma_init();
	boot_mark('i');
	//printf("[OK]\n");

	//printf("kernel: init semaphore         ... ");
	boot_mark('j');
	semaphore_init();
	boot_mark('k');
	//printf("[ok]\n");

	//printf("kernel: init irq               ... ");
	boot_mark('l');
	irq_init();
	boot_mark('m');
	//printf("[ok]\n");

	//printf("kernel: init share memory      ... ");
	boot_mark('n');
	shm_init();
	boot_mark('o');
	//printf("[ok]\n");

	//printf("kernel: init processes table   ... ");
	boot_mark('p');
	if(procs_init() != 0)
		halt();
	boot_mark('q');
	//printf("[ok] (%d)\n", _kernel_config.max_proc_num);

	boot_mark('r');
	printf("kernel: loading init ... ");
	if(load_init_proc() != 0)  {
		printf("[failed!]\n");
		halt();
	}
	boot_mark('s');
	printf("[ok]\n");

	kfork_core_halt(0);
#ifdef KERNEL_SMP
	_cpu_cores[0].actived = true;
	kernel_lock_init();
	printf("kernel: start cores ... 0");
	for(uint32_t i=1; i<_sys_info.cores; i++) {
		_cpu_cores[i].actived = false;
		kfork_core_halt(i);
		start_core(i);
		while(!_cpu_cores[i].actived)
			_delay_msec(10);
		printf(" %d", i);
	}
	printf("\n");
#endif

	//printf("kernel: set timer(fps): %6d ... ", _kernel_config.timer_freq);
	timer_set_interval(0, _kernel_config.timer_freq); 
	//printf("[ok]\n");
	//printf("kernel: start init process     ...\n"
		//   "---------------------------------------------------\n");

	__irq_enable();
	halt();

	kfree4k(_kernel_info.vsyscall_info);
}
