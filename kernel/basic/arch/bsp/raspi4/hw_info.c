#include <kernel/hw_info.h>
#include <kernel/system.h>
#include <kernel/kernel.h>
#include <kstring.h>

sys_info_t _sys_info;
uint32_t _allocatable_mem_top = 0;
uint32_t _allocatable_mem_base = 0;

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	strcpy(_sys_info.machine, "raspi4");
	_sys_info.phy_mem_size = 1024*MB;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.phy_base = 0xfe000000;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 16*MB;

  _allocatable_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_mem_top = _sys_info.phy_mem_size - 64*MB;
}

#define CORE0_BASE 0xff800000
void arch_vm(page_dir_entry_t* vm) {
	uint32_t offset = CORE0_BASE - _sys_info.mmio.phy_base; //CORE0_ROUTING
	uint32_t vbase = MMIO_BASE + offset;
	uint32_t pbase = _sys_info.mmio.phy_base + offset;
	map_pages(vm, vbase, pbase, pbase+16*KB, AP_RW_D, 1);

	/*offset = 0x00201000; //UART_OFFSET
	vbase = MMIO_BASE + offset;
	pbase = _sys_info.mmio.phy_base + offset;
	map_page(vm, vbase, pbase, AP_RW_D, 0);
	*/
}

