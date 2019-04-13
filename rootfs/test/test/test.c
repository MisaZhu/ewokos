#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syscall.h>
#include <ext2.h>

int32_t read_block(int32_t block, char* buf) {
	if(syscall2(SYSCALL_SDC_READ, block, (int32_t)buf) < 0)
		return -1;

	int32_t res = -1;
	while(true) {
		res = syscall0(SYSCALL_SDC_READ_DONE);
		if(res != 0)
			break;
		yield();
	}

	if(res < 0)
		return -1;
	return 0;
}

int main() {
	int32_t size;
	char* p = ext2_load("/sbin/init", read_block, malloc, &size);
		
	if(p != NULL)
		free(p);
	return 0;
}

