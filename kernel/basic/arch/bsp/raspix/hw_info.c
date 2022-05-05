#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>

sys_info_t _sys_info;
uint32_t _allocatable_mem_top = 0;
uint32_t _allocatable_mem_base = 0;

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

#ifdef PI2
	strcpy(_sys_info.machine, "raspi2");
	_sys_info.phy_mem_size = 1024*MB;
	_sys_info.mmio.phy_base = 0x3f000000;
#endif

#ifdef PI3A
	strcpy(_sys_info.machine, "raspi3");
	_sys_info.phy_mem_size = 512*MB;
	_sys_info.mmio.phy_base = 0x3f000000;
#endif

#ifdef PI3
	strcpy(_sys_info.machine, "raspi3");
	_sys_info.phy_mem_size = 1024*MB;
	_sys_info.mmio.phy_base = 0x3f000000;
#endif

#ifdef PI4
	strcpy(_sys_info.machine, "raspi4");
	_sys_info.phy_mem_size = 1024*MB;
	_sys_info.mmio.phy_base = 0xfe000000;
#endif

	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 16*MB;

	_allocatable_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_mem_top = _sys_info.phy_mem_size - 64*MB; //bcm283x framebuffer mem base
#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

#ifdef PI4
#define CORE0_BASE 0xff800000
#else
#define CORE0_BASE 0x40000000
#endif

void arch_vm(page_dir_entry_t* vm) {
	uint32_t offset = CORE0_BASE - _sys_info.mmio.phy_base; //CORE0_ROUTING
	uint32_t vbase = MMIO_BASE + offset;
	uint32_t pbase = _sys_info.mmio.phy_base + offset;
	map_pages(vm, vbase, pbase, pbase+16*KB, AP_RW_D, 1);

#ifdef PI2
	offset = 0x00201000; //UART_OFFSET
	vbase = MMIO_BASE + offset;
	pbase = _sys_info.mmio.phy_base + offset;
	map_page(vm, vbase, pbase, AP_RW_D, 0);
#endif
}

