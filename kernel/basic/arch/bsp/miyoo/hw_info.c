#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif

sys_info_t _sys_info;
uint32_t _allocatable_phy_mem_top = 0;
uint32_t _allocatable_phy_mem_base = 0;

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	strcpy(_sys_info.machine, "miyoo");
	_sys_info.phy_offset = 0x20000000;
	_sys_info.phy_mem_size = 128*MB;
	_sys_info.mmio.phy_base = 0x1f000000;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 16*MB;

	_allocatable_phy_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_phy_mem_top = _sys_info.phy_offset + _sys_info.phy_mem_size - 4*MB;
#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

void arch_vm(page_dir_entry_t* vm) {
	//map frame buffer
	uint32_t pfb = 0x27c00000;
	uint32_t vfb = 0x87c00000;
	map_pages_size(vm, vfb, pfb, 4*MB, AP_RW_D, 1);

	//map gic controller
	uint32_t pgic = 0x16000000;
	uint32_t vgic = 0x16000000;
	map_pages_size(vm, vgic, pgic, 4*MB, AP_RW_D, 1);


}

