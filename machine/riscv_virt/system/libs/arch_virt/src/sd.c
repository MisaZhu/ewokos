#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/syscall.h>
#include <ewoksys/mmio.h>
#include <sysinfo.h>

/**
 * initialize EMMC to read SDHC card
 */
int32_t virt_sd_init(void) {
	_mmio_base = mmio_map();
	syscall3(SYS_MEM_MAP, _mmio_base + 0x8000, 0xe0000000, 256*1024*1024);
	return 0;
}


int32_t virt_sd_read_sector(int32_t sector, void* buf) {
	memcpy(buf, (void*)(_mmio_base + 0x8000 + sector * 512L), 512);
	return 0;
}

int32_t virt_sd_write_sector(int32_t sector, const void* buf) {

	return 0;
}

