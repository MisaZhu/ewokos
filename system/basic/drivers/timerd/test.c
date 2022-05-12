#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/interrupt.h>
#include <sys/vdevice.h>
#include <sys/proto.h>

static uint32_t _v;

static void interrupt_handle(uint32_t interrupt) {
	_v++;
	sys_interrupt_end();
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	_v = 0;

	proto_t in;
	PF->init(&in)->addi(&in, 1000000)->addi(&in, (uint32_t)interrupt_handle);
	dev_cntl("/dev/timer", 0, &in, NULL);
	PF->clear(&in);

	while(1) {
		printf("pid: %d, v: %d\n", getpid(), _v);
		sleep(1);

		if(_v > 10)  {
			proto_t in;
			PF->init(&in)->addi(&in, 0)->addi(&in, 0);
			dev_cntl("/dev/timer", 0, &in, NULL);
			PF->clear(&in);
		}
	}
	return 0;
}
