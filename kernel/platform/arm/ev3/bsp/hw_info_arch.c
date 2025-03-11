#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>

uint32_t _allocable_phy_mem_top = 0;
uint32_t _allocable_phy_mem_base = 0;

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	strcpy(_sys_info.machine, "ev3");
	strcpy(_sys_info.arch, "armv6");

	_sys_info.phy_offset = 0xC0000000;
	_sys_info.vector_base = 0xC0000000;
	_sys_info.total_phy_mem_size = 64*MB;
	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	_sys_info.mmio.phy_base = 0x0;
	_sys_info.mmio.size = 32*MB;

	_sys_info.dma.size = DMA_SIZE;
    _allocable_phy_mem_base = V2P(get_allocable_start());
    _allocable_phy_mem_top = _sys_info.phy_offset +
            _sys_info.total_usable_mem_size -
            128*1024 -
            _sys_info.dma.size;
    _sys_info.dma.phy_base = _allocable_phy_mem_top;
    _sys_info.dma.v_base = DMA_BASE;

	_sys_info.cores = 1;
}

void arch_vm(page_dir_entry_t* vm) {
	 map_pages_size(vm, 0xFFFEE000, 0xFFFEE000, 8192, AP_RW_D, PTE_ATTR_DEV);
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
