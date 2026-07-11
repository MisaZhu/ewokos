#include <unistd.h>
#include <ewoksys/proc.h>

pid_t getppid(void) {
	procinfo_t info;

	if (proc_info(getpid(), &info) != 0) {
		return 0;
	}
	return info.father_pid;
}
