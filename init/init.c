#include <lib/stdio.h>
#include <lib/fork.h>
#include <lib/kserv.h>

void _start()
{
	putstr("\nLet there be light!\n  (here comes the first process 'init').\n");

	char buf[16];
	kservReg(KS_FS);

	int i = fork();
	if(i == 0) {
		putstr("child.\n");	
		exit(0);
	}
	else {
		kservWrite(KS_FS, "hello", 6);
		kservRead(KS_FS, buf, 16);
		kservWrite(KS_FS, "world", 6);
		kservWrite(KS_FS, "hello", 6);
		kservRead(KS_FS, buf, 16);
		putstr("father.\nbuffer=");	
		putstr(buf);
		putstr(".\n");	
	}
	exit(0);
}
