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


uint32_t _allocable_phy_mem_top = 0;
uint32_t _allocable_phy_mem_base = 0;
uint32_t _core_base_offset = 0;
uint32_t _uart_type = UART_MINI;
uint32_t _pi4 = 0;
	
#define FB_SIZE 64*MB
#define DMA_SIZE 256*KB

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	uint32_t pix_revision = bcm283x_board();
	_core_base_offset =  0x01000000;
	_pi4 = 0;
	_uart_type = UART_MINI;

	if(pix_revision == PI_4B_1G) {
		strcpy(_sys_info.machine, "raspberry-pi4b-1g");
		_sys_info.phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_4B_2G) {
		strcpy(_sys_info.machine, "raspberry-pi4b-2G");
		_sys_info.phy_mem_size = 2u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_4B_4G) {
		strcpy(_sys_info.machine, "raspberry-pi4b-4G");
		_sys_info.phy_mem_size = 4u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_4B_8G) {
		strcpy(_sys_info.machine, "raspberry-pi4b-8G");
		_sys_info.phy_mem_size = 4u*GB; //max for 32bits os
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_CM4_1G) {
		strcpy(_sys_info.machine, "raspberry-cm4-1g");
		_sys_info.phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_CM4_2G) {
		strcpy(_sys_info.machine, "raspberry-cm4-2G");
		_sys_info.phy_mem_size = 2u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_CM4_4G) {
		strcpy(_sys_info.machine, "raspberry-cm4-4G");
		_sys_info.phy_mem_size = 1u*GB;
		_sys_info.phy_offset = 16*MB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_CM3_1G) {
		strcpy(_sys_info.machine, "raspberry-cm3-1G");
		_sys_info.phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_2B) {
		strcpy(_sys_info.machine, "raspberry-pi2b");
		_sys_info.phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0x3f000000;
		_uart_type = UART_PL011;
	}
	else if(pix_revision == PI_3A) {
		strcpy(_sys_info.machine, "raspberry-pi3a");
		_sys_info.phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(pix_revision == PI_0_2W) {
		strcpy(_sys_info.machine, "raspberry-pi2w");
		_sys_info.phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(pix_revision == PI_3B) {
		strcpy(_sys_info.machine, "raspberry-pi3b");
		_sys_info.phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}

	if(_sys_info.phy_mem_size > (uint32_t)MAX_MEM_SIZE)
		_sys_info.phy_mem_size = MAX_MEM_SIZE;

	strcpy(_sys_info.arch, "armv7");
	_sys_info.phy_offset = 0;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 30*MB;

	_sys_info.dma.size = DMA_SIZE;
	_allocable_phy_mem_base = V2P(get_allocable_start()) + 16*MB;
	_allocable_phy_mem_top = _sys_info.phy_offset +
			_sys_info.phy_mem_size -
			FB_SIZE - 
			_sys_info.dma.size;
	_sys_info.dma.phy_base = _allocable_phy_mem_top;
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
