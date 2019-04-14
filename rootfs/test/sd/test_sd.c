#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syscall.h>
#include <ext2.h>

static int32_t read_block(int32_t block, char* buf) {
	if(syscall1(SYSCALL_SDC_READ, block) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = syscall1(SYSCALL_SDC_READ_DONE, (int32_t)buf);
		if(res == 0)
			break;
		yield();
	}
	return 0;
}

int main() {
	const char* fname = "/sbin/init";

	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg != NULL)
		fname = arg;

	int32_t size;
	char* p = ext2_load(fname, read_block, malloc, free, &size);
	if(p != NULL) {
		printf("read from sd card: %s, size:%d.\n", fname, size);
		free(p);
	}
	else {
		printf("read from sd card: %s failed!\n", fname);
	}
	return 0;
}

