#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/interrupt.h>

void int_func(int int_id, void* p) {
	printf("interrupt %d, i=%d\n", int_id, *(int*)p);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	int pid = fork();
	if(pid == 0) {
		int i = 0;
		proc_interrupt_setup(int_func, &i);
		while(1) {
			sleep(1);
			i++;
		}
	}
	else {
		while(1) {
			proc_interrupt(pid, 123);
			usleep(100000);
		}
	}
  return 0;
}

