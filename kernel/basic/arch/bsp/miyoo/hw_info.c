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
uint32_t _core_base_offset = 0;

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	_core_base_offset =  0x01000000;

	strcpy(_sys_info.machine, "miyoo-mini");
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
	uint32_t pfb = 0x27c00000;
	uint32_t vfb = 0x87c00000;
	map_pages_size(vm, vfb, pfb, 4*MB, AP_RW_D, 1);

	//map gic controller
	uint32_t pgic = 0x16000000;
	uint32_t vgic = 0x16000000;
	map_pages_size(vm, vgic, pgic, 4*MB, AP_RW_D, 1);

	uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset;
	uint32_t pbase = _sys_info.mmio.phy_base + _core_base_offset;
	map_page(vm, vbase, pbase, AP_RW_D, 1);
	map_page(vm, pbase, pbase, AP_RW_D, 1);
#ifdef KERNEL_SMP
	map_page(vm, SECOND_START_ADDR_HI , SECOND_START_ADDR_HI, AP_RW_D, 1);
#endif
}
