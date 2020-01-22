#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: kill <pid>.\n");
		return -1;
	}

	int pid = atoi(argv[1]);
	return syscall1(SYS_KILL, pid);
}

