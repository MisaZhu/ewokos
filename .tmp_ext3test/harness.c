#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* mimic the raspix kernel sd driver: lazy single-sector reads */
static FILE* _img;
static int32_t _sector;

int32_t sd_dev_read(int32_t sector) { _sector = sector; return 0; }

int32_t sd_dev_read_done(void* buf) {
	if(fseek(_img, (long)_sector * 512, SEEK_SET) != 0)
		return -1;
	return fread(buf, 512, 1, _img) == 1 ? 0 : -1;
}

int32_t sd_dev_read_blocks(int32_t sector, void* buf, uint32_t count) {
	uint8_t* out = (uint8_t*)buf;
	for(uint32_t i = 0; i < count; i++) {
		if(fseek(_img, (long)(sector + (int32_t)i) * 512, SEEK_SET) != 0)
			return -1;
		if(fread(out + i * 512, 512, 1, _img) != 1)
			return -1;
	}
	return 0;
}

/* kernel code under test */
#include "ext3read.h"
#include "ext2read.h"

static void test_read(const char* fname) {
	int32_t sz = -1;
	void* p = sd_read_ext3(fname, &sz);
	printf("sd_read_ext3(%s) -> %s, size=%d\n", fname, p ? "OK" : "NULL", sz);
	if(p && sz > 0) {
		uint32_t sum = 0;
		uint8_t* d = (uint8_t*)p;
		for(int32_t i = 0; i < sz; i++)
			sum = sum * 31 + d[i];
		printf("  checksum=%u\n", sum);
		free(p);
	}
}

int main(int argc, char** argv) {
	if(argc < 2) { printf("usage: %s image\n", argv[0]); return 1; }
	_img = fopen(argv[1], "rb");
	if(!_img) { perror("open"); return 1; }

	int32_t t = ext3_probe_fs();
	printf("ext3_probe_fs() = %d (2=ext2, 3=ext3)\n", t);

	test_read("/sbin/init");
	test_read("/etc/kernel/kernel.conf");
	fclose(_img);
	return 0;
}
