#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/kalloc.h>
#include <kstring.h>

extern char _framebuffer_base_raw[];
extern char _framebuffer_end_raw[];


uint32_t _allocable_phy_mem_top = 0;
uint32_t _allocable_phy_mem_base = 0;

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	strcpy(_sys_info.machine, "versatilepb");
	strcpy(_sys_info.arch, "armv6");

	_sys_info.phy_offset = 0;
	_sys_info.total_phy_mem_size = 256*MB;
	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	_sys_info.mmio.phy_base = 0x10000000;
	_sys_info.mmio.size = 4*MB;

	_sys_info.gpu.v_base = (uint32_t)_framebuffer_base_raw;
	_sys_info.gpu.phy_base = V2P(_sys_info.gpu.v_base);
	_sys_info.gpu.max_size = _framebuffer_end_raw - _framebuffer_base_raw;
	_sys_info.gpu.size = _sys_info.gpu.max_size;

	_allocable_phy_mem_base = V2P(get_allocable_start());
	_allocable_phy_mem_top = _sys_info.phy_offset +  
			_sys_info.total_usable_mem_size < _sys_info.mmio.phy_base ?
			_sys_info.total_usable_mem_size : _sys_info.mmio.phy_base;
	_sys_info.cores = 1;
}

void arch_vm(page_dir_entry_t* vm) {
	(void)vm;
}

void kalloc_arch(void) {
	kalloc_append(P2V(_allocable_phy_mem_base), P2V(_allocable_phy_mem_top));
}

int32_t  check_mem_map_arch(ewokos_addr_t phy_base, uint32_t size) {
	if(phy_base >= _sys_info.gpu.phy_base && size <= _sys_info.gpu.max_size)
		return 0;
	if(phy_base >= _sys_info.mmio.phy_base && size <= _sys_info.mmio.size)
		return 0;
	return -1;
}