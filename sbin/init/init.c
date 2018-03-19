#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <kserv/fs.h>

void _start() 
{
	if(fsInited() >= 0) { /*init process can only run at boot time.*/
		printf("Panic: 'init' process can only run at boot time!\n");
		exit(0);
	}

	/*file system kernel process*/
	printf("start file system ...\n");
	int pid = fork();
	if(pid == 0) { 
		exec("vfs");
	}

	while(fsInited() < 0) {
		yield();
	}
	printf("file system got ready.\n");

	/*shell process*/
	printf("start shell...\n");

	while(1) {
		pid = fork();
		if(pid == 0) {
			exec("shell");
		}
		else {
			wait(pid);
		}
	}
	exit(0);
}
