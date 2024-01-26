#include <unistd.h>
#include <ewoksys/proc.h>

int getpid(void) {
	return proc_getpid(-1);
}

