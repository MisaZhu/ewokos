#include <unistd.h>
#include <sys/proc.h>

int getpid(void) {
	return proc_getpid(-1);
}

