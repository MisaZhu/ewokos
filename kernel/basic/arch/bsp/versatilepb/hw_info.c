#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <kstring.h>

extern char _framebuffer_base_raw[];
extern char _framebuffer_end_raw[];

sys_info_t _sys_info;

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	strcpy(_sys_info.machine, "versatilepb");
	_sys_info.phy_mem_size = 256*MB;
	_sys_info.mmio.phy_base = 0x10000000;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 4*MB;

	_sys_info.fb.v_base = (uint32_t)_framebuffer_base_raw;
	_sys_info.fb.phy_base = V2P(_sys_info.fb.v_base);
	_sys_info.fb.size = _framebuffer_end_raw - _framebuffer_base_raw;
}

void arch_vm(page_dir_entry_t* vm) {
	(void)vm;
}

