#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/interrupt.h>
#include <sys/syscall.h>
#include <sys/ipc.h>

void int_func(int int_id, void* p) {
	uint64_t usec;
	uint32_t tic;
	syscall1(SYS_GET_KERNEL_USEC, (int32_t)&usec);
	tic = syscall0(SYS_GET_KERNEL_TIC);
	fprintf(stderr, "interrupt id=%d, i=%d, tic=%d, usec=%d\n", int_id, *(int*)p, tic, usec );
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;


	int i=0;
	/*proc_interrupt_setup(int_func, &i);
	int fd = open("/dev/timer0", O_RDWR);
	int msec = 100;
	write(fd, &msec, 4);
	*/

	proto_t arg, ret;
	proto_init(&arg, NULL, 0);
	proto_init(&ret, NULL, 0);
	while(1) {
		usleep(10000);
		proto_add_int(&arg, i);
		ipc_call(0, 0, &arg, &ret);
		proto_clear(&arg);
		printf("ret: %d\n", proto_read_int(&ret));
		proto_clear(&ret);
		i++;
	}
	//close(fd);
  return 0;
}

