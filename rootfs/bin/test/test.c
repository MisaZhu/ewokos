#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <syscall.h>

static int i;

void test(void* p) {
	while(true) {
		printf("child: %d, %x\n", i, (uint32_t)p);
		yield();
	}
}

void _start() {
	i = 0;

	int32_t tid = syscall2(SYSCALL_THREAD, (int32_t)test, 0x12345678);	
	printf("thread: %d\n", tid);
	while(true) {
		printf("father %d\n", i++);
		yield();
	}
	exit(0);
}

