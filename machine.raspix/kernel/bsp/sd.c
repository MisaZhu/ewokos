#include <bcm283x/sd.h>
#include <dev/sd.h>
#include <bcm283x/gpio.h>
#include "hw_arch.h"
#include <bcm283x/mailbox.h>
#include <kernel/hw_info.h>
#include <dev/timer.h>

uint32_t _sector = 0;

int32_t sd_init(void) {
	timer_init();
	if(_pi4){
		*(uint32_t*)(_sys_info.mmio.v_base + 0x2000d0) &= ~(0x2);
		//*(uint32_t*)(MMIO_BASE + 0x2000d0) |= 0x2;
	}
	return bcm283x_sd_init();
}

int32_t sd_dev_read(int32_t sector) {
	_sector = sector;
	return 0;
}

int32_t sd_dev_read_done(void* buf) {
	return bcm283x_sd_read(_sector, buf, 1);
}

int32_t sd_dev_write(int32_t sector, const void* buf) {
	return 0;
}

int32_t sd_dev_write_done(void) {
	return 0;
}
