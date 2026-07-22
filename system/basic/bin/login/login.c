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
#include <utils/telnet_console.h>
#include <setenv.h>

void putch(int c);

static int write_all_retry(int fd, const void* buf, size_t len) {
	const char* p = (const char*)buf;
	size_t off = 0;
	while(off < len) {
		ssize_t wr = write(fd, p + off, len - off);
		if(wr > 0) {
			off += (size_t)wr;
			continue;
		}
		if(errno == EAGAIN || errno == EINTR) {
			proc_usleep(1000);
			continue;
		}
		if(wr == 0 && errno == 0)
			errno = EPIPE;
		return -1;
	}
	return 0;
}

static telnet_console_t _telnet_console;

static session_info_t* check(const char* user, const char* password, int* res) {
	static session_info_t info;
	*res = session_check(user, password, &info);
	if(*res == 0)
		return &info;
	return NULL;
}

static void input(str_t* s, bool show) {
	bool telnet = telnet_console_is_active();

	str_reset(s);
	char c;
	while(true) {
		int i = telnet ? telnet_console_read(0, &_telnet_console, &c) : read(0, &c, 1);
		if(i <= 0) {
			int err = errno;
			if(err == EAGAIN || err == EINTR) {
				proc_usleep(30000);
				continue;
			}
			break;
		}

		c = telnet ? telnet_console_parse(&_telnet_console, (uint8_t)c) : (uint8_t)c;
		if(c == 0) {
			continue;
		}

		if(!telnet && c == '\r')
			c = '\n';

		if (c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
			if (s->len > 0) {
				//delete last char
				if(show) {
					static const char erase_seq[3] = { CONSOLE_LEFT, ' ', CONSOLE_LEFT };
					(void)write_all_retry(1, erase_seq, sizeof(erase_seq));
				}
				s->len--;
			}
		}
		else {
			char out = c;
			if(!show && c != '\n')
				out = '*';

			(void)write_all_retry(1, &out, 1);

			if(c == '\n')
				break;		

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
	telnet_console_init(&_telnet_console);

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
		str_free(password);

		if(info == NULL || info->cmd[0] == 0) {
			str_free(user);
			return -1;
		}

		setenv("USER", user->cstr);
		str_free(user);

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
