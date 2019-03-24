#include <types.h>
#include <stdio.h>
#include <thread.h>
#include <unistd.h>

static int i;

void test(void* p) {
	while(true) {
		printf("child: %d, %x\n", i, (uint32_t)p);
		yield();
	}
}

void _start() {
	i = 0;

	int32_t tid = thread_raw(test, (void*)0x12345678);	
	printf("thread: %d\n", tid);
	while(i < 1000) {
		printf("father %d\n", i++);
		yield();
	}
	exit(0);
}

