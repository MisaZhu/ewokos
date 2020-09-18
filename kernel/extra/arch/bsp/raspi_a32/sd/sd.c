#include "bcm283x/sd.h"

/**
 * initialize EMMC to read SDHC card
 */
int32_t sd_init(void) {
	return bcm283x_sd_init();
}	

int32_t sd_dev_read(int32_t sector) {
	return bcm283x_sd_read(sector);
}

int32_t sd_dev_read_done(void* buf) {
	return bcm283x_sd_read_done(buf);
}
