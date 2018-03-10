#include <types.h>
#include <stdio.h>
#include <unistd.h>

void _start() 
{
	/*file system kernel process*/
	printf("start file system ...\n");
	int pid = fork();
	if(pid == 0) { 
		exec("vfs");
	}

	/*shell process*/
	printf("start shell...\n");
	pid = fork();
	if(pid == 0) {
		exec("shell");
	}

	while(1) {
		yield();
	}
	exit(0);
}
