#include "mario.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

void* interrupterThread(void* arg) {
	vm_t* vm = (vm_t*)arg;
	
	pthread_detach(pthread_self());
	
	int32_t count = 0;

	while(true) {
		//call interrupter 'onInterrupt' with 2 argument.
		interrupt_by_name(vm, vm->root, "onInterrupt", str_from_int(count, 10));

		usleep(100);
		count++;
	}
	return NULL;
}

const char* js = " \
	function onInterrupt(count) { \
		debug(\"interrupter: \" + count); \
	} \
	while(true) { \
		yield(); \
	}";

int main(int argc, char** argv) {
	vm_t* vm = vm_new(compile);
	vm_init(vm, NULL, NULL);

	pthread_t pth;
	pthread_create(&pth, NULL, interrupterThread, vm);

	vm_load_run(vm, js);
	
	vm_close(vm);
	return 0;
}
