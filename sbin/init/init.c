#include <types.h>
#include <stdio.h>
#include <fork.h>

void _start() 
{
	printf("start file system ...\n");
	int pid = fork();
	if(pid == 0) { //file system process
		exec("vfs");
	}

	printf("start shell...\n");
	pid = fork();
	if(pid == 0) {
		while(1) {
			pid = fork();
			if(pid == 0) { //shell process
				exec("shell");
			}
			wait(pid);
		}
	}

	while(1) {
		yield();
	}
	exit(0);
}
