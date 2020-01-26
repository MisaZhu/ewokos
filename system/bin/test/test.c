#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/interrupt.h>

void int_func(int int_id, void* p) {
	printf("interrupt id=%d, pid=%d, i=%d\n", int_id, getpid(), *(int*)p);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;


	int i=0;
	proc_interrupt_setup(int_func, &i);
	int fd = open("/dev/timer0", O_RDWR);
	int msec = 100;
	write(fd, &msec, 4);

	while(1) {
		sleep(1);
		i++;
	}
	close(fd);
  return 0;
}

