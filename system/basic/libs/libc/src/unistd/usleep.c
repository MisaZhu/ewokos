#include <unistd.h>
#include <ewoksys/syscall.h>

int usleep(unsigned int usecs) {
	if(usecs == 0)
		syscall0(SYS_YIELD);
	else
		syscall1(SYS_USLEEP, usecs);
	return 0;
}

