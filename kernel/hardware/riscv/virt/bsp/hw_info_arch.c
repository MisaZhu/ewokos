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

	strcpy(_sys_info.machine, "virt");
	strcpy(_sys_info.arch, "risc-v rv64");
	_sys_info.phy_offset = 0x80000000;
	_sys_info.vector_base = 0x81000000;
	_sys_info.phy_mem_size = 128*MB;
	_sys_info.mmio.phy_base = 0x10000000;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 32*KB;
	_sys_info.dma.size = 256*1024;

	_allocatable_phy_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_phy_mem_top = _sys_info.phy_offset +
			_sys_info.phy_mem_size -
			_sys_info.dma.size;
	_sys_info.dma.phy_base = _allocatable_phy_mem_top;

#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

#define INITRD_RAM_VADDR 0xC0008000
#define INITRD_RAM_PADDR 0xE0000000
#define INITRD_RAM_SIZE	0x4000000

void arch_vm(page_dir_entry_t* vm) {
	map_pages(vm, INITRD_RAM_VADDR, INITRD_RAM_PADDR, INITRD_RAM_PADDR + INITRD_RAM_SIZE, AP_RW_D, PTE_ATTR_WRBACK);
}
