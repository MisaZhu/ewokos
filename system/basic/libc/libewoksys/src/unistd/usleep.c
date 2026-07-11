#include <unistd.h>
#include <ewoksys/proc.h>

int usleep(useconds_t usec) {
	proc_usleep(usec);
	return 0;
}
