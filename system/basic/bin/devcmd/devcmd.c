#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/keydef.h>
#include <ewoksys/mstr.h>
#include <ewoksys/proc.h>

void putch(int c);

int32_t cmd_gets(int fd, str_t* buf) {
	str_reset(buf);	

	while(1) {
		char c, old_c;
		int i = read(fd, &c, 1);
		if(i <= 0 || c == 0) {
		 	if(i == 0)
			 	return -1;
			proc_usleep(10000);
			continue;
		}

		if (c == KEY_BACKSPACE) {
			if (buf->len > 0) {
				//delete last char
				putch(CONSOLE_LEFT); 
				putch(' ');
				putch(CONSOLE_LEFT); 
				buf->len--;
			}
		}
		else if (c == CONSOLE_LEFT) {
			if (buf->len > 0) {
				//delete last char
				putch(c); 
				buf->len--;
			}
		}
		else {
			if(c == '\r') {
				old_c = c;
				c = '\n';
			}
			else  {
				old_c = 0;
				if(c == '\n' && old_c == '\r')
					continue;
			}

			putch(c);
			if(c == '\n')
				break;
			if(c > 27)
				str_addc(buf, c);
		}
	}
	str_addc(buf, 0);
	return 0;
}

static void prompt(const char* dev_name) {
	printf("devcmd: %s> ", dev_name);
}

int devcmd(const char* dev_name) {
	str_t* cmd = str_new("");
	while(true) {
		prompt(dev_name);
		cmd_gets(0, cmd);
		if(strcmp(cmd->cstr, "exit") == 0)
			break;

		char* ret = dev_cmd(dev_name, cmd->cstr);
		if(ret == NULL) {
			printf("command excute failed!\n");
		}
		else {
			printf("%s", ret);
			free(ret);
		}
	}

	str_free(cmd);
	return 0;
}

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL);

	char* ret;
	if(argc < 2) {
		printf("Usage: devcmd <dev> [cmd]\n");
		return -1;
	}

	if(argc < 3) {
		return devcmd(argv[1]);
	}

	str_t* args = str_new("");
	for(int i=2; i<argc; i++) {
		str_add(args, argv[i]);
		str_add(args, " ");
	}

	ret = dev_cmd(argv[1], args->cstr);
	str_free(args);

	if(ret == NULL) {
		printf("devcmd failed!\n");
		return -1;
	}

	printf("%s", ret);
	free(ret);
	return 0;
}
