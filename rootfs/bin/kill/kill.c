#include <cmain.h>
#include <stdlib.h>
#include <syscall.h>

int main() {
	init_cmain_arg();
	const char* arg = read_cmain_arg();
	arg = read_cmain_arg();
	if(arg == NULL) {
		return -1;
	}

	int pid = atoi(arg);
	return syscall2(SYSCALL_SYSTEM_CMD, 3, pid);
}

