#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <kernel/system.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <stdbool.h>
#include <bcm283x/board.h>
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

void sys_info_init_arch(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	uint32_t pix_revision = bcm283x_board();
	_sys_info.total_phy_mem_size = 512*MB;
	_core_base_offset =  0x01000000;
	_sys_info.mmio.phy_base = 0x3f000000;
	_pi4 = 0;
	_uart_type = UART_MINI;

	if(pix_revision == PI_4B_1G) {
		strcpy(_sys_info.machine, "raspberry-pi4b-1g");
		_sys_info.total_phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_4B_2G) {
		strcpy(_sys_info.machine, "raspberry-pi4b-2G");
		_sys_info.total_phy_mem_size = 2u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_4B_4G) {
		strcpy(_sys_info.machine, "raspberry-pi4b-4G");
		_sys_info.total_phy_mem_size = 4u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_4B_8G) {
		strcpy(_sys_info.machine, "raspberry-pi4b-8G");
		_sys_info.total_phy_mem_size = 4u*GB; //max for 32bits os
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_CM4_1G) {
		strcpy(_sys_info.machine, "raspberry-cm4-1g");
		_sys_info.total_phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_CM4_2G) {
		strcpy(_sys_info.machine, "raspberry-cm4-2G");
		_sys_info.total_phy_mem_size = 2u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_CM4_4G) {
		strcpy(_sys_info.machine, "raspberry-cm4-4G");
		_sys_info.total_phy_mem_size = 2u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_CM3_1G) {
		strcpy(_sys_info.machine, "raspberry-cm3-1G");
		_sys_info.total_phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0xfe000000;
		_core_base_offset =  0x01800000;
		_pi4 = 1;
	}
	else if(pix_revision == PI_2B) {
		strcpy(_sys_info.machine, "raspberry-pi2b");
		_sys_info.total_phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0x3f000000;
		_uart_type = UART_PL011;
	}
	else if(pix_revision == PI_3A) {
		strcpy(_sys_info.machine, "raspberry-pi3a");
		_sys_info.total_phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(pix_revision == PI_0_2W) {
		strcpy(_sys_info.machine, "raspberry-pi2w");
		_sys_info.total_phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(pix_revision == PI_3B) {
		strcpy(_sys_info.machine, "raspberry-pi3b");
		_sys_info.total_phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(pix_revision == PI_5_2G) {
		strcpy(_sys_info.machine, "raspberry-pi5");
		_sys_info.total_phy_mem_size = 1u*GB;
		_sys_info.mmio.phy_base = 0x7c000000;
		_uart_type = UART_PL011;
	}

	_sys_info.total_usable_mem_size = _sys_info.total_phy_mem_size;
	if(_sys_info.total_usable_mem_size > (uint32_t)MAX_USABLE_MEM_SIZE)
		_sys_info.total_usable_mem_size = MAX_USABLE_MEM_SIZE;

#if __aarch64__
	strcpy(_sys_info.arch, "aarch64");
#elif __arm__
	strcpy(_sys_info.arch, "armv7");
#endif
	_sys_info.mmio.size = 31*MB;

	_sys_info.gpu.v_base = FB_BASE;
	_sys_info.gpu.max_size = FB_SIZE;

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
	uint32_t vbase = _sys_info.mmio.v_base + _core_base_offset;
	uint32_t pbase = _sys_info.mmio.phy_base + _core_base_offset;
	map_page(vm, vbase, pbase, AP_RW_RW, PTE_ATTR_DEV);
	//map_page(vm, pbase, pbase, AP_RW_D, PTE_ATTR_DEV);
}


#ifdef KERNEL_SMP
extern char __entry[];
void start_core(uint32_t core_id) {
    if(core_id >= _sys_info.cores)
        return;
#if __arm__
    uint32_t core_start_addr = (core_id * 0x10 + 0x8c) + 
       _sys_info.mmio.v_base + 
       _core_base_offset;

    put32(core_start_addr, __entry);
#elif __aarch64__
	uint64_t core_start_addr = 0x800000E0 + (core_id - 1) * 8;
	*(volatile uint32_t*)core_start_addr = (uint32_t)__entry;
	flush_dcache();
#endif
    __asm__("sev");
}
#endif

void kalloc_arch(void) {
	if(_sys_info.total_usable_mem_size > 1*GB) {
		//skip framebuffer mem block
		//kalloc_append(P2V(_allocable_phy_mem_base), P2V(_sys_info.gpu.phy_base));
		kalloc_append(P2V(_allocable_phy_mem_base), P2V(0x3c100000));
		kalloc_append(P2V(1*GB), P2V(_allocable_phy_mem_top));
	}
	else
		kalloc_append(P2V(_allocable_phy_mem_base), P2V(_allocable_phy_mem_top));
}

int32_t  check_mem_map_arch(ewokos_addr_t phy_base, uint32_t size) {
	if(phy_base >= _sys_info.gpu.phy_base && size <= _sys_info.gpu.max_size)
		return 0;
	if(phy_base >= _sys_info.mmio.phy_base && size <= _sys_info.mmio.size)
		return 0;
	return -1;
}
