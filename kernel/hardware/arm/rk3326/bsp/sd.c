#include <dev/sd.h>
#include <mm/mmu.h>

static uint8_t *sector_buf = (void*)0x3e000000;

extern int dwmci_init(void);
int32_t sd_init(void) {
	return dwmci_init();
}

extern int mmc_read_blocks(void *dst, uint32_t sector);
int32_t sd_dev_read(int32_t sector) {
	return mmc_read_blocks(sector_buf, sector);
}

int32_t sd_dev_read_done(void* buf) {
	memcpy(buf, sector_buf, 512);
	return 0;
}

int32_t sd_dev_write(int32_t sector, const void* buf) {
	return -1;
}

int32_t sd_dev_write_done(void) {
	return -1;
}
