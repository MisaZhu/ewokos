#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <arch/bcm283x/mmc.h>

int32_t bcm283x_sd_init(void) {
	_mmio_base = mmio_map();
	mmc_init();
	return 0;
}

int32_t bcm283x_sd_read_sector(int32_t sector, void* buf) {
	mmc_read_blocks(buf, sector, 1);
	return 0;
}

int32_t bcm283x_sd_write_sector(int32_t sector, const void* buf) {
	mmc_write_blocks(sector, 1, buf);
	return 0;
}

