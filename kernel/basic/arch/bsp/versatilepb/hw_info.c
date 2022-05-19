#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <kstring.h>

extern char _framebuffer_base_raw[];
extern char _framebuffer_end_raw[];

sys_info_t _sys_info;
uint32_t _allocatable_mem_top = 0;
uint32_t _allocatable_mem_base = 0;

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	strcpy(_sys_info.machine, "versatilepb");
	_sys_info.phy_mem_size = 256*MB;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.phy_mem_start = 0;
	_sys_info.mmio.phy_base = 0x10000000;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 4*MB;

	_sys_info.fb.v_base = (uint32_t)_framebuffer_base_raw;
	_sys_info.fb.phy_base = V2P(_sys_info.fb.v_base);
	_sys_info.fb.size = _framebuffer_end_raw - _framebuffer_base_raw;

	_allocatable_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_mem_top = 
			_sys_info.phy_mem_size < _sys_info.mmio.phy_base ?
			_sys_info.phy_mem_size : _sys_info.mmio.phy_base;
	_sys_info.cores = 1;
}

void arch_vm(page_dir_entry_t* vm) {
	(void)vm;
}

