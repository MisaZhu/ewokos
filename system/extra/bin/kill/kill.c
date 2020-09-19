#include <stdlib.h>
#include <stdio.h>
#include <sys/syscall.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: kill <pid>.\n");
		return -1;
	}

	int pid = atoi(argv[1]);
	int res = syscall2(SYS_EXIT, pid, 0);
	if(res != 0) {
		printf("Error, can not kill proc: %d!\n", pid);
		return -1;
	}
	return 0;
}

