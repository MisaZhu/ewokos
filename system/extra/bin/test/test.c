#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


void* do_thread(void* p) {
	(void)p;
	while(1) {
		printf("child: %d\n", pthread_self());
		sleep(1);
	}
	return NULL;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	pthread_create(NULL, NULL, do_thread, NULL);
	while(1) {
		printf("parent: %d\n", getpid());
		sleep(1);
	}
	return 0;
}
