#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <stdbool.h>

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif


uint32_t _allocable_phy_mem_top = 0;
uint32_t _allocable_phy_mem_base = 0;
uint32_t _core_base_offset = 0;
uint32_t _pi4 = 0;
	
#define FB_SIZE 64*MB

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	_sys_info.phy_offset = 0x40000000;
	_sys_info.vector_base = 0x40000000;
	_sys_info.total_phy_mem_size = 512*MB;
	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	_core_base_offset =  0x01000000;
	_sys_info.mmio.phy_base = 0x8000000;
	strcpy(_sys_info.machine, "virt");

	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	if(_sys_info.total_usable_mem_size > (uint32_t)MAX_USABLE_MEM_SIZE)
		_sys_info.total_usable_mem_size = MAX_USABLE_MEM_SIZE;

#if __aarch64__
	strcpy(_sys_info.arch, "aarch64");
#elif __arm__
	strcpy(_sys_info.arch, "armv7");
#endif
	_sys_info.mmio.size = 128*MB;

	_allocable_phy_mem_base = V2P(get_allocable_start());
	_sys_info.sys_dma.phy_base = _allocable_phy_mem_base;
	_sys_info.sys_dma.size = DMA_SIZE;
	_sys_info.sys_dma.v_base = DMA_V_BASE;
	_allocable_phy_mem_base += DMA_SIZE;

	if(_sys_info.total_usable_mem_size <= 1*GB) {
		_allocable_phy_mem_top = _sys_info.phy_offset +
				_sys_info.total_usable_mem_size - FB_SIZE;
	}
	else {
		_allocable_phy_mem_top = _sys_info.phy_offset + _sys_info.total_usable_mem_size;
	}

#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif

#if __aarch64__
	_sys_info.vector_base = (ewokos_addr_t)&interrupt_table_start;
#endif 
}

void arch_vm(page_dir_entry_t* vm) {
}


#ifdef KERNEL_SMP
extern char __entry[];
void start_core(uint32_t core_id) {
}
#endif

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
