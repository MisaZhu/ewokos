#include <bcm283x/sd.h>
#include <dev/sd.h>
#include <mm/mmu.h>

int32_t sd_init(void) {
	/*
	* https://forums.raspberrypi.com/viewtopic.php?p=1953489
	* raspberry-pi's official github repo has some commentary on 
	* switching to the old EMMC controller 
	*/
	 *(uint32_t*)(MMIO_BASE + 0x2000d0) |= 0x2;
	return bcm283x_sd_init();
}

int32_t sd_dev_read(int32_t sector) {
	return bcm283x_sd_read(sector);
}

int32_t sd_dev_read_done(void* buf) {
	return bcm283x_sd_read_done(buf);
}

int32_t sd_dev_write(int32_t sector, const void* buf) {
	return bcm283x_sd_write(sector, buf);
}

int32_t sd_dev_write_done(void) {
	return bcm283x_sd_write_done();
}
