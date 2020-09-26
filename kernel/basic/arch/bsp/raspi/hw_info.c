#include <kernel/hw_info.h>
#include <kernel/system.h>
#include <kstring.h>

sys_info_t _sys_info;

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	strcpy(_sys_info.machine, "raspi");
	_sys_info.phy_mem_size = 512*MB;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.phy_base = 0x20000000;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 16*MB;
}

#define AUX_OFFSET 0x00215000
void arch_vm(page_dir_entry_t* vm) {
	uint32_t offset = AUX_OFFSET;
	uint32_t vbase = _sys_info.mmio.v_base + offset;
	uint32_t pbase = _sys_info.mmio.phy_base + offset;
	map_page(vm, vbase, pbase, AP_RW_D, 0);
}
