#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vfs/fs.h>
#include <kserv.h>
#include <syscall.h>

void _start() {
	if(getpid() > 0) {
		printf("Panic: 'init' process can only run at boot time!\n");
		exit(0);
	}

	int pid = 0;

	printf("start vfs service ... ");
	pid = fork();
	if(pid == 0) { 
		exec("vfsd");
	}
	kservWait("kserv.vfsd");
	printf("ok.\n");

	printf("start initfs service ... ");
	pid = fork();
	if(pid == 0) { 
		exec("initfs");
	}
	kservWait("dev.initfs");

	pid = fork();
	if(pid == 0) { 
		printf("start tty service ... ");
		exec("ttyd");
	}
	kservWait("dev.tty");

	pid = fork();
	if(pid == 0) { 
		printf("start framebuffer service ... ");
		exec("fbd");
	}
	kservWait("dev.fb");

	pid = fork();
	if(pid == 0) { 
		printf("start user manager ... ");
		exec("userman");
	}
	kservWait("kserv.userman");
	printf("ok.\n");

	pid = fork();
	if(pid == 0) { 
		printf("start console service ... ");
		exec("consoled");
	}
	kservWait("dev.console");

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
