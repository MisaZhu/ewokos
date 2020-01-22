#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/interrupt.h>

void int_func(int int_id, void* p) {
	if(int_id >= US_INT_USER_DEF)
		printf("user   interrupt id=%d, i=%d\n", int_id, *(int*)p);
	else
		printf("kernel interrupt id=%d, i=%d\n", int_id, *(int*)p);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	int pid = fork();
	if(pid == 0) {
		int i = 0;
		proc_interrupt_setup(int_func, &i);
		proc_interrupt_register(US_INT_KERNEL_TIC);
		while(1) {
			sleep(1);
			i++;
		}
	}
	else {
		while(1) {
			proc_interrupt(pid, 123);
			sleep(1);
		}
	}
  return 0;
}

