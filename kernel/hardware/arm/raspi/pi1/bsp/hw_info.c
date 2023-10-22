#include <kernel/hw_info.h>
#include <kernel/system.h>
#include <kernel/kernel.h>
#include <bcm283x/framebuffer.h>
#include <kstring.h>

sys_info_t _sys_info;
uint32_t _allocatable_phy_mem_top = 0;
uint32_t _allocatable_phy_mem_base = 0;

#define FB_SIZE 64*MB
#define DMA_SIZE 256*KB

void sys_info_init(void) {
	memset(&_sys_info, 0, sizeof(sys_info_t));

	strcpy(_sys_info.machine, "raspi1");
	strcpy(_sys_info.arch, "armv6");
	_sys_info.phy_offset = 0;
	_sys_info.phy_mem_size = 512*MB;
	_sys_info.kernel_base = KERNEL_BASE;
	_sys_info.mmio.phy_base = 0x20000000;
	_sys_info.mmio.v_base = MMIO_BASE;
	_sys_info.mmio.size = 16*MB;

	_sys_info.dma.size = DMA_SIZE;

	_allocatable_phy_mem_base = V2P(ALLOCATABLE_MEMORY_START);
	_allocatable_phy_mem_top = _sys_info.phy_offset +
			_sys_info.phy_mem_size -
			FB_SIZE - 
			_sys_info.dma.size;
	_sys_info.dma.phy_base = _allocatable_phy_mem_top;
	_sys_info.cores = 1;
}

#define AUX_OFFSET 0x00215000
void arch_vm(page_dir_entry_t* vm) {
	uint32_t offset = AUX_OFFSET;
	uint32_t vbase = _sys_info.mmio.v_base + offset;
	uint32_t pbase = _sys_info.mmio.phy_base + offset;
	map_page(vm, vbase, pbase, AP_RW_D, PTE_ATTR_DEV);

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
