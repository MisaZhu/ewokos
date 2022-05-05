#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t _lock;
static uint32_t _v;

void* do_thread(void* p) {
	(void)p;
	while(1) {
		pthread_mutex_lock(&_lock);
		_v++;
		printf("c: v= %d\n", _v);
		pthread_mutex_unlock(&_lock);
	}
	return NULL;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	_v = 0;

	pthread_mutex_init(&_lock, NULL);

	pthread_create(NULL, NULL, do_thread, NULL);
	while(1) {
		pthread_mutex_lock(&_lock);
		_v++;
		printf("p: v= %d\n", _v);
		pthread_mutex_unlock(&_lock);
	}
	return 0;
}
