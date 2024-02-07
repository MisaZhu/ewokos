#include <unistd.h>
#include <ewoksys/proc.h>

int usleep(uint32_t usec) {
	proc_usleep(usec);
}