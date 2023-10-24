#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <stdbool.h>
#include <bcm283x/board.h>
#include <bcm283x/framebuffer.h>
#include "hw_arch.h"

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif

sys_info_t _sys_info;
uint32_t _allocatable_phy_mem_top = 0;
uint32_t _allocatable_phy_mem_base = 0;
uint32_t _core_base_offset = 0;
uint32_t _uart_type = UART_MINI;
uint32_t _pi4 = 0;

static bool isPi4B1G(uint32_t revision) {
	return (revision == 0xa03111 ||
			revision == 0xa03112 ||
			revision == 0xa03115);
}

static bool isPi4B2G(uint32_t revision) {
	return (revision == 0xb03111 ||
			revision == 0xb03112 ||
			revision == 0xb03114 ||
			revision == 0xa03115);
}

static bool isPi4B4G(uint32_t revision) {
	return (revision == 0xc03111 ||
			revision == 0xc03112 ||
			revision == 0xc03114 ||
			revision == 0xc03115);
}

static bool isPi4B8G(uint32_t revision) {
	return (revision == 0xd03114 ||
			revision == 0xd03115);
}

static bool isPi2B(uint32_t revision) {
	return (revision == 0xa01040 ||
			revision == 0xa01041 ||
			revision == 0xa21041 ||
			revision == 0xa02042 ||
			revision == 0xa22042);
}

static bool isPi3A(uint32_t revision) {
	return (revision == 0x9020e0);
}

static bool isPi2W(uint32_t revision) {
	return (revision == 0x902120);
}

static bool isPi3B(uint32_t revision) {
	return (revision == 0xa020a0 ||
			revision == 0xa02082 ||
			revision == 0xa020d3 ||
			revision == 0xa22082 ||
			revision == 0xa220a0 ||
			revision == 0xa02100 ||
			revision == 0xa32082 ||
			revision == 0xa52082 ||
			revision == 0xa22083);
}
	
#define FB_SIZE 64*MB
#define DMA_SIZE 256*KB

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	uint32_t pix_revision = bcm283x_board();
	_core_base_offset =  0x01000000;
	_pi4 = 0;

	if(isPi4B1G(pix_revision)) {
		strcpy(_sys_info.machine, "raspberry-pi4b-1g");
		_sys_info.phy_mem_size = 1024*MB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(isPi4B2G(pix_revision)) {
		strcpy(_sys_info.machine, "raspberry-pi4b-2G");
		_sys_info.phy_mem_size = 2048*MB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(isPi4B4G(pix_revision) || isPi4B8G(pix_revision)) {
		strcpy(_sys_info.machine, "raspberry-pi4b-4G");
		_sys_info.phy_mem_size = 4096*MB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(isPi2B(pix_revision)) {
		strcpy(_sys_info.machine, "raspberry-pi2b");
		_sys_info.phy_mem_size = 1024*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
		_uart_type = UART_PL011;
	}
	else if(isPi3A(pix_revision)) {
		strcpy(_sys_info.machine, "raspberry-pi3a");
		_sys_info.phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(isPi2W(pix_revision)) {
		strcpy(_sys_info.machine, "raspberry-pi2w");
		_sys_info.phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(isPi3B(pix_revision)) {
		strcpy(_sys_info.machine, "raspberry-pi3b");
		_sys_info.phy_mem_size = 1024*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}

	strcpy(_sys_info.arch, "armv7");
	_sys_info.phy_offset = 0;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 16*MB;

	_sys_info.dma.size = DMA_SIZE;
	_allocatable_phy_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_phy_mem_top = _sys_info.phy_offset +
			_sys_info.phy_mem_size -
			FB_SIZE - 
			_sys_info.dma.size;
	_sys_info.dma.phy_base = _allocatable_phy_mem_top;
#ifdef KERNEL_SMP
	_sys_info.cores = get_cpu_cores();
#else
	_sys_info.cores = 1;
#endif
}

void arch_vm(page_dir_entry_t* vm) {
	uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset;
	uint32_t pbase = _sys_info.mmio.phy_base + _core_base_offset;
	map_page(vm, vbase, pbase, AP_RW_D, PTE_ATTR_DEV);
	map_page(vm, pbase, pbase, AP_RW_D, PTE_ATTR_DEV);

#ifdef KCONSOLE
	fbinfo_t* fb_info = bcm283x_get_fbinfo();
	if(fb_info->pointer != 0) {
		map_pages_size(vm, 
			fb_info->pointer,
			V2P(fb_info->pointer),
			fb_info->size_max,
			AP_RW_D, PTE_ATTR_DEV);
	}
#endif

	if(_pi4)
	 	*(uint32_t*)(MMIO_BASE + 0x2000d0) |= 0x2;
}


#ifdef KERNEL_SMP
extern char __entry[];
void start_core(uint32_t core_id) {
    if(core_id >= _sys_info.cores)
        return;

    uint32_t core_start_addr = (core_id * 0x10 + 0x8c) + 
       _sys_info.mmio.phy_base + 
       _core_base_offset;

    put32(core_start_addr, __entry);
    __asm__("sev");
}
#endif
