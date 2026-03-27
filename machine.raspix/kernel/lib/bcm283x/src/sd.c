#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <bcm283x/mmc.h>


int32_t bcm283x_sd_init(void) {
	mmc_init();
	return 0;
}

int32_t bcm283x_sd_read(int32_t sector, void* buf, int count) {
	//printf("read sector %d, count %d\n", sector, count);
	mmc_read_blocks(buf, sector, count);
	return 0;
}

int32_t bcm283x_sd_write(int32_t sector, const void* buf, int count) {
	return -1;
}

