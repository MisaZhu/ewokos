#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/thread.h>

static uint32_t _v;

void* do_thread(void* p) {
	(void)p;
	while(1) {
		thread_lock();
		_v++;
		printf("c: v= %d\n", _v);
		thread_unlock();
	}
	return NULL;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	_v = 0;

	pthread_create(NULL, NULL, do_thread, NULL);
	while(1) {
		thread_lock();
		_v++;
		printf("p: v= %d\n", _v);
		thread_unlock();
	}
	return 0;
}
