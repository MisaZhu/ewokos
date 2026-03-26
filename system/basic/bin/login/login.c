#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <ewoksys/session.h>
#include <ewoksys/mstr.h>
#include <ewoksys/core.h>
#include <ewoksys/keydef.h>
#include <ewoksys/vfs.h>
#include <setenv.h>

void putch(int c);

static session_info_t* check(const char* user, const char* password, int* res) {
	static session_info_t info;
	*res = session_check(user, password, &info);
	if(*res == 0)
		return &info;
	return NULL;
}

//remove telnet command add crlr
static uint8_t telnet_parse(uint8_t c){
	static int state = 0;
	uint8_t ret = 0;
	switch(state){
		case 0:
			if(c == 0xFF){
				state = 1;
			}else if(c == '\r'){
				state = 3;
				ret = '\n';
			}else{
				ret = c;
			}
			break;
		case 1:
			if(c >= 0xFB && c <= 0xFE)
				state = 2;
			else
				state = 0;
			break;
		case 2:
			state = 0;
			break;
		case 3:
			if(c != '\n')
				ret = c;
			state = 0;
			break;
		default:
			state = 0;
			break;
	}

	return ret;
}

static void input(str_t* s, bool show) {
	str_reset(s);
	char c, old_c;
	while(true) {
		int i = read(0, &c, 1);
		c = telnet_parse(c);
		if(c == 0)
			continue;
		
		if(i <= 0 || c == 0) {
		 	if(errno != EAGAIN)
			//if(i == 0)
			 	break;
			proc_usleep(30000);
			continue;
		}	

		if (c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
			if (s->len > 0) {
				//delete last char
				if(show) {
					putch(CONSOLE_LEFT);
					putch(' ');
					putch(CONSOLE_LEFT);
				}
				s->len--;
			}
		}
		else {
			if(show || c == '\n')
				write(1, &c, 1);
	
			if(c == '\n')
				break;		

			if(!show) {
				char x = '*';
				write(1, &x, 1);
			}

			if(c > 27)
				str_addc(s, c);
		}
	}
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	const char* console = getenv("CONSOLE_ID");
	if(console == NULL)
		console = "-";

	setbuf(stdout, NULL);
	//session_info_t* info = check("root", "");  //if root have no password, run shell directly
	session_info_t* info = NULL;
	if(info == NULL || info->cmd[0] == 0) {
		str_t* user = str_new("root");
		str_t* password = str_new("");
		printf("[%s] login: ", console);
		input(user, true);
		if(user->len > 0) {
			int res = 0;
			info = check(user->cstr, password->cstr, &res); 
			if(info == NULL && res != SESSION_ERR_USR) {
				printf("[%s] password: ", console);
				input(password, false);
				info = check(user->cstr, password->cstr, &res); 
			}
		}

		str_free(user);
		str_free(password);

		if(info == NULL || info->cmd[0] == 0)
			return -1;

		//vfs_create(info->home, NULL, FS_TYPE_DIR, 0750, false, true);
		//chown(info->home, info->uid, info->gid);

		int res = setgid(info->gid);
		if(res != 0) {
			fprintf(stderr, "Error, setgid failed!\n");
			return -1;
		}

		res = setuid(info->uid);
		if(res != 0) {
			fprintf(stderr, "Error, setuid failed!\n");
			return -1;
		}
	}

	setenv("HOME", info->home);
	proc_exec(info->cmd);
	return 0;
}
