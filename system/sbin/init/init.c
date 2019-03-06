#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kserv/fs.h>
#include <kserv/kserv.h>
#include <syscall.h>

void _start() {
	if(fsInited() >= 0) { /*init process can only run at boot time.*/
		printf("Panic: 'init' process can only run at boot time!\n");
		exit(0);
	}

	//initfs process
	printf("start initfs service ...\n");
	int pid = fork();
	if(pid == 0) { 
		exec("initfs");
	}

	while(fsInited() < 0)
		yield();
	printf("initfs got ready.\n");

	pid = fork();
	if(pid == 0) { 
		//ttyd process
		printf("start ttyd ...\n");
		exec("ttyd");
	}
	kservWait("dev.tty");
	printf("ttyd got ready.\n");

	pid = fork();
	if(pid == 0) { 
		//userman kernel process
		printf("start user manager ...\n");
		exec("userman");
	}
	kservWait("USER_MAN");
	printf("user manager got ready.\n");

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
