#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/interrupt.h>

static uint32_t _v;

static void interrupt_handle(uint32_t interrupt) {
	_v++;
	sys_interrupt_end();
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	_v = 0;

	sys_interrupt_setup(SYS_INT_TIMER0, interrupt_handle);

	while(1) {
		printf("v: %d\n", _v);
		sleep(1);
	}
	return 0;
}
