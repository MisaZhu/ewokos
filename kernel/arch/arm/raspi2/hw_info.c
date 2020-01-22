#include <kernel/hw_info.h>
#include <kernel/system.h>
#include <dev/framebuffer.h>
#include <kstring.h>
#include "mailbox.h"

static hw_info_t _hw_info;

void hw_info_init(void) {
	strcpy(_hw_info.machine, "raspi2");
	_hw_info.phy_mem_size = 512*MB;
	_hw_info.phy_mmio_base = 0x3F000000;
	_hw_info.mmio_size = 4*MB;
}

inline hw_info_t* get_hw_info(void) {
	return &_hw_info;
}

#define CORE0_ROUTING 0x40000000
void arch_vm(page_dir_entry_t* vm) {
	uint32_t offset = CORE0_ROUTING - _hw_info.phy_mmio_base;
	uint32_t vbase = MMIO_BASE + offset;
	uint32_t pbase = _hw_info.phy_mmio_base + offset;
	map_pages(vm, vbase, pbase, pbase+16*KB, AP_RW_D);
}

void hw_optimise(void) {
}
