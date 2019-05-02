#include <cmain.h>
#include <stdlib.h>
#include <syscall.h>

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;

	int pid = atoi(argv[1]);
	return syscall2(SYSCALL_SYSTEM_CMD, 3, pid);
}

