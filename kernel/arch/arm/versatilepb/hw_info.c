#include <kernel/hw_info.h>
#include <mm/mmu.h>
#include <dev/framebuffer.h>
#include <kstring.h>

static hw_info_t _hw_info;

void hw_info_init(void) {
	strcpy(_hw_info.machine, "versatilepb");
	_hw_info.phy_mem_size = 256*MB;
	_hw_info.phy_mmio_base = 0x10000000;
	_hw_info.mmio_size = 4*MB;
}

inline hw_info_t* get_hw_info(void) {
	return &_hw_info;
}

void arch_vm(page_dir_entry_t* vm) {
	(void)vm;
}

void hw_optimise(void) {
}
