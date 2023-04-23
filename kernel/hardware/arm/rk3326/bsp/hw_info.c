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
	strcpy(_sys_info.machine, "rk3326");
	_sys_info.phy_offset = 0x0;
	_sys_info.vector_base = 0x100000;
	_sys_info.phy_mem_size = 128*MB;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.phy_base = 0xFF000000;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 8*MB;

	_allocatable_phy_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_phy_mem_top = _sys_info.phy_offset + _sys_info.phy_mem_size;
#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

#ifdef KERNEL_SMP
extern char __entry[];
inline void __attribute__((optimize("O0"))) start_core(uint32_t core_id) { //TODO
    if(core_id >= _sys_info.cores)
        return;
	uint32_t entry = V2P((uint32_t)(__entry) + _sys_info.kernel_base);

    __asm__("sev");
}
#endif

void arch_vm(page_dir_entry_t* vm) {
	//map framebuffer
	map_pages_size(vm, 0x3de00000, 0x3de00000, 0x2200000, AP_RW_D, PTE_ATTR_WRBACK_ALLOCATE);	
}
