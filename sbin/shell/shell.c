#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define KEY_BACKSPACE 127
#define KEY_LEFT 0x8

static void gets(char* buf, int len) {
	int i = 0;
	while(true) {
		if(i >= len)
			break;

		int c = getch();
		if(c == 0) {
			yield();
			continue;
		}

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

static int handle(const char* cmd) {
	if(strcmp(cmd, "cd") == 0) {
		chdir("/");
		return 0;
	}
	if(strncmp(cmd, "cd ", 3) == 0) {
		char* p = cmd + 3;
		if(p[0] == '/') {
			chdir(p);
			return 0;
		}

		char cwd[FNAME_MAX];
		if(getcwd(cwd, FNAME_MAX) == NULL)
			return -1;

		int len = strlen(cwd);
		if(cwd[len-1] != '/') {
			cwd[len] = '/';
			len++;
		}

		strcpy(cwd+len, p);
		printf("cd: [%s]\n", cwd);
		chdir(cwd);
		return 0;
	}

	return -1; /*not shell internal command*/
}

#define CMD_MAX 128

void _start() {
	printf(
			"\n: Hey! wake up!\n"
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

	char cmd[CMD_MAX];
	char cwd[FNAME_MAX];

	while(1) {
		printf("%s $ ", getcwd(cwd, FNAME_MAX));
		gets(cmd, CMD_MAX);
		if(cmd[0] == 0)
			continue;

		if(strcmp(cmd, "exit") == 0)
			break;

		if(handle(cmd) == 0)
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
	exit(0);
}
