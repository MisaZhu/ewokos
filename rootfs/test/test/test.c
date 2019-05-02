#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syscall.h>
#include <device.h>

int main() {
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg == NULL) {
		return -1;
	}
	
	int i = open(arg, O_RDWR|O_CREAT);
	if(i >= 0)
		close(i);
	return 0;
}

