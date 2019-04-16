#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <vfs/fs.h>
#include <kserv.h>
#include <syscall.h>

int main() {
	if(getpid() > 0) {
		printf("Panic: 'init' process can only run at boot time!\n");
		return -1;
	}

	int pid = 0;

	printf("start vfs service ... ");
	pid = fork();
	if(pid == 0) { 
		exec("/sbin/vfsd");
	}
	kserv_wait("kserv.vfsd");
	printf("ok.\n");

	pid = fork();
	if(pid == 0) { 
		exec("/sbin/dev/sdcard");
	}
	kserv_wait("dev.sdcard");

	pid = fork();
	if(pid == 0) { 
		//printf("start tty service ... ");
		exec("/sbin/dev/ttyd");
	}
	kserv_wait("dev.tty");

	pid = fork();
	if(pid == 0) { 
		//printf("start framebuffer service ... ");
		exec("/sbin/dev/fbd");
	}
	kserv_wait("dev.fb");

	pid = fork();
	if(pid == 0) { 
		//printf("start proc service ... ");
		exec("/sbin/dev/procd");
	}
	kserv_wait("dev.proc");

	pid = fork();
	if(pid == 0) { 
		printf("start user manager ... ");
		exec("/sbin/userman");
	}
	kserv_wait("kserv.userman");
	printf("ok.\n");

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
			exec("/sbin/login");
		}
		else {
			wait(pid);
		}
	}
	return 0;
}
