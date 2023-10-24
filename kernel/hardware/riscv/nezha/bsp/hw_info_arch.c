#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif


uint32_t _allocatable_phy_mem_top = 0;
uint32_t _allocatable_phy_mem_base = 0;
uint32_t _core_base_offset = 0;


void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	_core_base_offset =  0x01000000;

	strcpy(_sys_info.machine, "nezha");
	strcpy(_sys_info.arch, "risc-v rv64");
	_sys_info.phy_mem_size = 1*GB;
	_sys_info.phy_offset = 0x40000000;
	_sys_info.vector_base = 0x41000000;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.phy_base = 0x0;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 128*MB;
	_sys_info.dma.size = 256*KB;

	_allocatable_phy_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_phy_mem_top = _sys_info.phy_offset +
			_sys_info.phy_mem_size -
			_sys_info.dma.size - 32*MB;
	_sys_info.dma.phy_base = _allocatable_phy_mem_top;

#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

void arch_vm(page_dir_entry_t* vm) {

}
