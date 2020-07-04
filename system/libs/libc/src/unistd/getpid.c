#include <unistd.h>
#include <sys/proto.h>

int getpid(void) {
	return proc_getpid(-1);
}

