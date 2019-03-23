#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <syscall.h>

void test(bool father) {
	while(true) {
		if(father)
			printf("father\n");
		else
			printf("child\n");
	}
}

void _start() {
	int32_t tid = syscall0(SYSCALL_THREAD);	
	if(tid == 0)
		test(false);
	else
		test(true);
	exit(0);
}

