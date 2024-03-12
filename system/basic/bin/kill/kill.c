#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

int main(int argc, char* argv[]) {
	int pid, res, sig=SIGSTOP;
	if(argc < 2) {
		printf("Usage: kill <pid> {sig}.\n");
		return -1;
	}

	pid = atoi(argv[1]);
	if(argc > 2) 
		sig = atoi(argv[2]);

	res = kill(pid, sig);
	if(res != 0) {
		printf("Error, can not kill proc: %d!\n", pid);
		return -1;
	}
	return 0;
}

