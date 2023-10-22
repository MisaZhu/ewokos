#include <kernel/hw_info.h>
#include <kernel/kernel.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <stdbool.h>
#include <bcm283x/board.h>
#include <bcm283x/framebuffer.h>

#ifdef KERNEL_SMP
#include <kernel/core.h>
#endif

sys_info_t _sys_info;
uint32_t _allocatable_phy_mem_top = 0;
uint32_t _allocatable_phy_mem_base = 0;
uint32_t _core_base_offset = 0;

#ifdef PI4
static bool isPi4B1G(uint32_t revision) {
	return (revision == 0xa03111);
}
#else
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
#endif
	
#define FB_SIZE 64*MB
#define DMA_SIZE 256*KB

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));
	uint32_t pix_revision = bcm283x_board();

#ifdef PI4
	strcpy(_sys_info.machine, "raspi4B1G");
	_sys_info.phy_mem_size = 1024*MB;
	_sys_info.mmio.phy_base = 0xfe000000;
	_core_base_offset =  0x01800000;
#else
	_core_base_offset =  0x01000000;
	if(isPi2B(pix_revision)) {
		strcpy(_sys_info.machine, "raspi2B");
		_sys_info.phy_mem_size = 1024*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(isPi3A(pix_revision)) {
		strcpy(_sys_info.machine, "raspi3A");
		_sys_info.phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(isPi2W(pix_revision)) {
		strcpy(_sys_info.machine, "raspi2w");
		_sys_info.phy_mem_size = 512*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
	else if(isPi3B(pix_revision)) {
		strcpy(_sys_info.machine, "raspi3B");
		_sys_info.phy_mem_size = 1024*MB;
		_sys_info.mmio.phy_base = 0x3f000000;
	}
#endif
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
