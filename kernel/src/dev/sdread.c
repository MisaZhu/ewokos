#include <ext2.h>
#include <kstring.h>
#include <dev/sdc.h>
#include <mm/kmalloc.h>

static char buf1[SDC_BLOCK_SIZE];
static char buf2[SDC_BLOCK_SIZE];

static int32_t read_block(int32_t block, char* buf) {
	if(sdc_read_block(block) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = sdc_read_done(buf);
		if(res == 0)
			break;
	}
	return 0;
}

char* from_sd(const char *filename, int32_t* sz) { 
	return ext2_load(filename, sz, km_alloc, read_block, buf1, buf2);
}
