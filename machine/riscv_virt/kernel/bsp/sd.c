#include <dev/sd.h>
#include <kernel/hw_info.h>
#include "cfi.h"


#define INITRD_RAM_ADDR (0xc0000000 + 0x8000)
flash_info_t  finfo; 

static uint32_t gCacheSector = 0;
int32_t sd_init(void) {
	// finfo.base = 0xC0008000;
	// finfo.chipwidth = 1;
	// finfo.portwidth = 2;
	// finfo.chip_lsb = 0;
	// finfo.start[0] = finfo.base;
	// flash_detect_cfi(&finfo, 0);
	return 0;
}

int32_t sd_dev_read(int32_t sector) {
	gCacheSector = sector; 
	return 0;
}

int32_t sd_dev_read_done(void* buf) {
	memcpy(buf, (void*)(INITRD_RAM_ADDR + gCacheSector * 512L), 512);
	return 0;
}

int32_t sd_dev_write(int32_t sector, const void* buf) {
	return 0;
}

int32_t sd_dev_write_done(void) {
	return 0;
}
