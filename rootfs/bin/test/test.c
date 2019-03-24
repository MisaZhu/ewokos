#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <syscall.h>

static int i;

void test() {
	while(true) {
		printf("child: %d\n", i);
		yield();
	}
}

void _start() {
	i = 0;

	int32_t tid = syscall1(SYSCALL_THREAD, (int32_t)test);	
	printf("thread: %d\n", tid);
	while(true) {
		printf("father %d\n", i++);
		yield();
	}
	exit(0);
}

