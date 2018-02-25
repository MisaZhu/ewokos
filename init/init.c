#include <lib/stdio.h>
#include <lib/fork.h>
#include <lib/kserv.h>

void _start()
{
	putstr("\nLet there be light!\n  (here comes the first process 'init').\n");
	while(true) {
		int c = getch();
		if(c != 0)
			putch(c);
		if(c == '\r' || c == '\n')
			break;
	}

	char buf[16];
	kservReg(KS_FS);

	int i = fork();
	if(i == 0) {
		while(1) {
			kservRequest(KS_FS, "hello", 6);
//			kservGetResponse(KS_FS, buf, 16);
			putstr("child: buffer=");	
/*			putstr(buf);
			putstr(".\n");	
			*/
		}
		exit(0);
	}

	while(1) {
		int client;
		kservGetRequest(KS_FS, &client, buf, 16);
		putstr("father: buffer=");	
		putstr(buf);
		putstr(".\n");	
		kservResponse(KS_FS, "world", 6);
	}
}
