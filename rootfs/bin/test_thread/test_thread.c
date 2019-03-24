#include <types.h>
#include <stdio.h>
#include <thread.h>
#include <unistd.h>

static int i;

void test(void* p) {
	while(true) {
		printf("child thread: %d, arg = '%s'\n", i, (const char*)p);
		yield();
	}
}

void _start() {
	i = 0;

	int32_t tid = thread_raw(test, (void*)"thread arg data");
	if(tid < 0)
		exit(0);

	while(i < 1000) {
		printf("main thread %d\n", i++);
		yield();
	}
	exit(0);
}

