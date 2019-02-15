#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <kserv/fs.h>
#include <syscall.h>

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

	/*userman kernel process*/
	printf("start user manager ...\n");
	pid = fork();
	if(pid == 0) { 
		exec("userman");
	}

	printf("\n: Hey! wake up!\n"
			": Matrix had you.\n"
			": Follow the rabbit...\n\n");

	printf(
			"    ,-.,-.\n"
			"    ( ( (\n"
			"    \\ ) ) _..-.._\n"
			"   __)/,’,’       `.\n"
			" ,'     `.     ,--.  `.\n"
			",'   @        .’    `  \\\n"
			"(Y            (         ;’’.\n" 
			" `--.____,     \\        ,  ;\n"
			" ((_ ,----’ ,---’    _,’_,’\n"
			"  (((_,- (((______,-’\n\n"); 

	/*set uid to root*/
	syscall2(SYSCALL_SET_UID, getpid(), 0);
	/*start login process*/
	while(1) {
		pid = fork();
		if(pid == 0) {
			exec("login");
		}
		else {
			wait(pid);
		}
	}
	exit(0);
}
