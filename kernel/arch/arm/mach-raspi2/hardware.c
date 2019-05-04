#include <hardware.h>
#include <mm/mmu.h>
#include <mailbox.h>

void hw_init() {
	mailboxInit();
}

uint32_t get_phy_ram_size() {
	TagsInfoT info;
	if(mailboxGetBoardInfo(&info) == NULL)
		return 256*MB;
	return info.memory_arm_size+info.memory_vc_size;
}

uint32_t get_mmio_base_phy() {
	return 0x3F000000;
}

uint32_t get_initrd_base_phy() {
	return 0x08000000;
}

uint32_t get_initrd_size() {
	return 1*MB;
}

uint32_t get_mmio_mem_size() {
	return 4*MB;
}

uint32_t get_uart_irq() {
	return 25;
}

uint32_t get_timer_irq() {
	return 0;
}

#define CORE0_ROUTING 0x40000000

void arch_set_kernel_vm(page_dir_entry_t* vm) {
	uint32_t offset = CORE0_ROUTING - get_mmio_base_phy();
	uint32_t vbase = MMIO_BASE + offset;
	uint32_t pbase = get_mmio_base_phy() +offset;
	map_pages(vm, vbase, pbase, pbase+16*KB, AP_RW_D);
}
