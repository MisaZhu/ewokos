#include <stdio.h>
#include <string.h>
#include <fork.h>

#define KEY_BACKSPACE 127
#define KEY_LEFT 0x8

void gets(char* buf, int len) {
	int i = 0;
	while(true) {
		if(i >= len)
			break;

		int c = getch();
		if(c == 0)
			continue;

		if (c == KEY_BACKSPACE) {
			if (i > 0) {
				//delete last char
				putch(KEY_LEFT); 
				putch(' ');
				putch(KEY_LEFT); 
				i--;
			}
		}
		else {
			putch(c);
			if(c == '\r' || c == '\n')
				break;
			buf[i] = c;
			i++;
		}
	}
	buf[i] = 0;
}

void _start() {
	printf(
			": Hey! wake up!\n"
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

	char cmd[32];
	while(1) {
		printf("$ ");
		gets(cmd, 31);
		if(cmd[0] == 0)
			continue;

		int child_pid = fork();
		if (child_pid == 0) {
			if(exec(cmd) != 0) {
				printf("unknown command: '%s'.\n", cmd);
				exit(0);
			}
		}
		else {
			wait(child_pid);
		}
	}
}
