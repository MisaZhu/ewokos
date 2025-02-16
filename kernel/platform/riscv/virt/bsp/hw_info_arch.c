#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif


uint32_t _allocable_phy_mem_top = 0;
uint32_t _allocable_phy_mem_base = 0;
uint32_t _core_base_offset = 0;


void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	_core_base_offset =  0x01000000;

	strcpy(_sys_info.machine, "virt");
	strcpy(_sys_info.arch, "risc-v rv64");
	_sys_info.phy_offset = 0x80000000;
	_sys_info.vector_base = 0x81000000;
	_sys_info.total_phy_mem_size = 128*MB;
	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	_sys_info.mmio.phy_base = 0x10000000;
	
	_sys_info.mmio.size = 32*KB;
	_sys_info.dma.size = 256*1024;

	_allocable_phy_mem_base = V2P(get_allocable_start());
	_allocable_phy_mem_top = _sys_info.phy_offset +
			_sys_info.total_usable_mem_size -
			_sys_info.dma.size;
	_sys_info.dma.phy_base = _allocable_phy_mem_top;
	_sys_info.dma.v_base = DMA_BASE;

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

void kalloc_arch(void) {
	kalloc_append(P2V(_allocable_phy_mem_base), P2V(_allocable_phy_mem_top));
}

int32_t  check_mem_map_arch(uint32_t phy_base, uint32_t size) {
	if(phy_base >= _sys_info.fb.phy_base && size <= _sys_info.fb.size)
		return 0;
	if(phy_base >= _sys_info.mmio.phy_base && size <= _sys_info.mmio.size)
		return 0;
	return -1;
}