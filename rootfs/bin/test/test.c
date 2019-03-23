#include <types.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>
#include <syscall.h>

static int i;

void test() {
	while(true) {
		printf("child %x, %d\n", &i, i);
		yield();
	}
	exit(0);
}

void _start() {
	i = 0;

	int32_t tid = syscall1(SYSCALL_THREAD, (int32_t)test);	
	while(true) {
		printf("father %x, %d\n", &i, i++);
		yield();
	}
	exit(0);
}

