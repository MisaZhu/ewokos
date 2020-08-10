#include <mm/mmu.h>
#include <kernel/hw_info.h>
#include <dev/actled.h>
#include <kprintf.h>

void act_led_flash(void) {
	act_led(true);
	_delay(1000000);
	act_led(false);
	_delay(1000000);
}

void boot_init(void) {
	hw_info_init();
	_mmio_base = get_hw_info()->phy_mmio_base;
	act_led_flash();
}
