#include <types.h>
#include <stdio.h>
#include <thread.h>
#include <unistd.h>
#include <semaphore.h>

static int i;
static semaphore_t s;

void test(void* p) {
	while(true) {
		semaphore_lock(&s);
		printf("child thread: %d, arg = '%s'\n", i++, (const char*)p);
		semaphore_unlock(&s);
		yield();
	}
}

void _start() {
	i = 0;
	semaphore_init(&s);

	int32_t tid = thread_raw(test, (void*)"thread arg data");
	if(tid < 0)
		exit(0);

	while(i < 1000) {
		semaphore_lock(&s);
		printf("main thread %d\n", i++);
		semaphore_unlock(&s);
		yield();
	}

	semaphore_close(&s);
	exit(0);
}

