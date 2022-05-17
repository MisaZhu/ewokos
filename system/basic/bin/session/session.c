#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	while(1) {
		int pid = fork();
		if(pid == 0) {
			if(exec("/bin/login") < 0) {
				exit(-1);
			}
		}
		else {
			waitpid(pid);
		}
	}
	return 0;
}
