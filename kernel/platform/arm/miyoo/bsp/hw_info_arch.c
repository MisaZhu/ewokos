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

#define FB_SIZE 4*MB

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	_core_base_offset =  0x01000000;
	strcpy(_sys_info.machine, "miyoo-mini");
	strcpy(_sys_info.arch, "armv7");
	_sys_info.phy_offset = 0x20000000;
	_sys_info.vector_base = 0x20000000;
	_sys_info.total_phy_mem_size = 128*MB;
	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	_sys_info.mmio.phy_base = 0x1f000000;
	_sys_info.mmio.size = 16*MB;

	_sys_info.dma.size = DMA_SIZE;

	_allocable_phy_mem_base = V2P(get_allocable_start());
	_allocable_phy_mem_top = _sys_info.phy_offset +
			_sys_info.total_usable_mem_size -
			FB_SIZE - 
			_sys_info.dma.size;
	_sys_info.dma.phy_base = _allocable_phy_mem_top;
	_sys_info.dma.v_base = DMA_BASE;

	_sys_info.fb.phy_base = 0x27c00000; 
	_sys_info.fb.v_base = 0x87c00000; 
	_sys_info.fb.size = FB_SIZE; 

#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

#ifdef KERNEL_SMP
#define SECOND_START_ADDR_HI      0x1F20404C
#define SECOND_START_ADDR_LO      0x1F204050
#define SECOND_MAGIC_NUMBER_ADDR  0x1F204058
extern char __entry[];
inline void __attribute__((optimize("O0"))) start_core(uint32_t core_id) { //TODO
    if(core_id >= _sys_info.cores)
        return;
	uint32_t entry = V2P((uint32_t)(__entry) + _sys_info.kernel_base);

	do {
 	   put32(SECOND_START_ADDR_HI, (entry >> 16));
	} while(get32(SECOND_START_ADDR_HI) != (entry >> 16));

	do {
   		put32(SECOND_START_ADDR_LO, (entry & 0xffff));
	} while(get32(SECOND_START_ADDR_LO) != (entry & 0xffff));

	do {
 	   put32(SECOND_MAGIC_NUMBER_ADDR, 0xBABE);
	} while(get32(SECOND_MAGIC_NUMBER_ADDR) != 0XBABE);
    __asm__("sev");
}
#endif

void arch_vm(page_dir_entry_t* vm) {
	//map frame buffer
	map_pages_size(vm, _sys_info.fb.v_base, _sys_info.fb.phy_base, _sys_info.fb.size, AP_RW_D, PTE_ATTR_DEV);

	//map gic controller
	uint32_t pgic = 0x16000000;
	uint32_t vgic = 0x16000000;
	map_pages_size(vm, vgic, pgic, 4*MB, AP_RW_D, PTE_ATTR_DEV);

	uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset;
	uint32_t pbase = _sys_info.mmio.phy_base + _core_base_offset;
	map_page(vm, vbase, pbase, AP_RW_D, PTE_ATTR_DEV);
	map_page(vm, pbase, pbase, AP_RW_D, PTE_ATTR_DEV);
#ifdef KERNEL_SMP
	map_page(vm, SECOND_START_ADDR_HI , SECOND_START_ADDR_HI, AP_RW_D, PTE_ATTR_DEV);
#endif
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