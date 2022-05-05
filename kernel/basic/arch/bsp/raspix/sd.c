#include <dev/mmio.h>
#include <bcm283x/sd.h>
#include <dev/sd.h>

int32_t sd_init(void) {
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
