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
