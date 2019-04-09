#include <hardware.h>
#include <mm/mmu.h>
#include <dev/fb.h>

void hw_init() {
}

uint32_t get_phy_ram_size() {
	return 256*MB;
}

uint32_t get_mmio_base_phy() {
	return 0x10000000;
}

uint32_t get_mmio_mem_size() {
	return 4*MB;
}

uint32_t get_initrd_base_phy() {
	return 0x08000000;
}

uint32_t get_initrd_size() {
	return 2*MB;
}

uint32_t get_uart_irq() {
	return 12;
}

uint32_t get_timer_irq() {
	//Timer0 and Timer1 interrupt at IRQ4. Timer2 and Timer3 interrupt at IRQ5
	return 5;
}

void arch_set_kernel_vm(page_dir_entry_t* vm) {
	(void)vm;
	uint32_t fbBase = (uint32_t)V2P(_fb_start); //framebuffer addr
	map_pages(vm, fbBase, fbBase, fbBase+2*MB, AP_RW_D);
}
