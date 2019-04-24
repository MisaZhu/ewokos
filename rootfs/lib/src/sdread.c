#include <ext2.h>
#include <syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <devices.h>

static char buf1[SDC_BLOCK_SIZE];
static char buf2[SDC_BLOCK_SIZE];

static int32_t _typeid = dev_typeid(DEV_SDC, 0);

static int32_t read_block(int32_t block, char* buf) {
	if(syscall2(SYSCALL_DEV_BLOCK_READ, _typeid, block) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = syscall2(SYSCALL_DEV_BLOCK_READ_DONE, _typeid, (int32_t)buf);
		if(res == 0)
			break;
		yield();
	}
	return 0;
}

char* from_sd(const char *filename, int32_t* sz) { 
	return ext2_load(filename, sz, malloc, read_block, buf1, buf2);
}
