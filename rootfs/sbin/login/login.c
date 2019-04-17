#include <unistd.h>
#include <stdio.h>
#include <syscall.h>
#include <userman.h>

#define KEY_BACKSPACE 127
#define KEY_LEFT 0x8

static void gets(char* buf, int len, bool disp) {
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
				if(disp) {
					putch(KEY_LEFT); 
					putch(' ');
					putch(KEY_LEFT); 
				}
				i--;
			}
		}
		else {
			if(c == '\r' || c == '\n') {
				putch(c);
				break;
			}
			if(disp) {
				putch(c);
			}
			buf[i] = c;
			i++;
		}
	}
	buf[i] = 0;
}


int main() {
	while(1) {
		char user[128];
		char passwd[128];

		printf("login: ");
		gets(user, 127, true);
		if(user[0] == 0)
			continue;

		printf("password: ");
		gets(passwd, 127, false);

		int uid = usermanAuth(user, passwd);
		if(uid >= 0) {
			syscall2(SYSCALL_SET_UID, getpid(), uid);
			break;
		}
	}
	printf("\n");
	exec("/sbin/shell");
	return 0;
}
