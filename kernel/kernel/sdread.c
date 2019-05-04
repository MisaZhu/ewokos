#include <ext2.h>
#include <kstring.h>
#include <dev/basic_dev.h>
#include <mm/kmalloc.h>

static char buf1[SDC_BLOCK_SIZE];
static char buf2[SDC_BLOCK_SIZE];

static int32_t read_block(int32_t block, char* buf) {
	uint32_t typeid = dev_typeid(DEV_SDC, 0);
	if(dev_block_read(typeid, block) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = dev_block_read_done(typeid, buf);
		if(res == 0)
			break;
	}
	return 0;
}

char* from_sd(const char *filename, int32_t* sz) { 
	return ext2_load(filename, sz, km_alloc, read_block, buf1, buf2);
}
