#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/fork.h>

#define KEY_BACKSPACE 127

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
				putch(27); putch(91); putch(68); //3 codes to be : arrow left 
				putch(' ');
				putch(27); putch(91); putch(68); //3 codes to be : arrow left 
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
	char cmd[32];
	while(1) {
		putstr("ewok:> ");
		gets(cmd, 31);

		int child_pid = fork();
		if (child_pid == 0) {
			exec(cmd);
		}
		else {
			wait(child_pid);
		}
	}
}
