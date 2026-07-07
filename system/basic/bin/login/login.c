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

#define TELNET_IAC  255
#define TELNET_DONT 254
#define TELNET_DO   253
#define TELNET_WONT 252
#define TELNET_WILL 251

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3

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

static int is_telnet_console(void) {
	const char* cid = getenv("CONSOLE_ID");
	return cid != NULL && strcmp(cid, "telnet") == 0;
}

static int _telnet_raw_buf_off = 0;
static int _telnet_raw_buf_len = 0;
static uint8_t _telnet_raw_buf[256];

static int telnet_read_raw(int fd, char* c) {
	if(_telnet_raw_buf_off < _telnet_raw_buf_len) {
		*c = (char)_telnet_raw_buf[_telnet_raw_buf_off++];
		return 1;
	}

	int ret = read(fd, _telnet_raw_buf, sizeof(_telnet_raw_buf));
	if(ret <= 0)
		return ret;

	_telnet_raw_buf_off = 1;
	_telnet_raw_buf_len = ret;
	*c = (char)_telnet_raw_buf[0];
	return 1;
}

static session_info_t* check(const char* user, const char* password, int* res) {
	static session_info_t info;
	*res = session_check(user, password, &info);
	if(*res == 0)
		return &info;
	return NULL;
}

static void telnet_reply_option(uint8_t verb, uint8_t opt) {
	uint8_t reply[3] = { TELNET_IAC, 0, opt };
	int len = 3;
	int supported = (opt == TELNET_OPT_ECHO || opt == TELNET_OPT_SUPPRESS_GA);

	switch(verb) {
	case TELNET_DO:
		if(!supported) {
			reply[1] = TELNET_WONT;
		}
		else {
			len = 0;
		}
		break;
	case TELNET_DONT:
		len = 0;
		break;
	case TELNET_WILL:
		if(!supported) {
			reply[1] = TELNET_DONT;
		}
		else {
			len = 0;
		}
		break;
	case TELNET_WONT:
		len = 0;
		break;
	default:
		len = 0;
		break;
	}

	if(len > 0) {
		(void)write(1, reply, len);
	}
}

//remove telnet command add crlr
static uint8_t telnet_parse(uint8_t c){
	static int state = 0;
	static uint8_t verb = 0;
	uint8_t ret = 0;
	switch(state){
		case 0:
			if(c == TELNET_IAC){
				state = 1;
			}else if(c == '\r'){
				state = 3;
				ret = '\n';
			}else{
				ret = c;
			}
			break;
		case 1:
			if(c >= TELNET_WILL && c <= TELNET_DONT) {
				verb = c;
				state = 2;
			}
			else
				state = 0;
			break;
		case 2:
			telnet_reply_option(verb, c);
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
	bool telnet = is_telnet_console();

	str_reset(s);
	char c;
	while(true) {
		int i = telnet ? telnet_read_raw(0, &c) : read(0, &c, 1);
		if(i <= 0) {
			int err = errno;
			if(err == EAGAIN || err == EINTR) {
				proc_usleep(30000);
				continue;
			}
			break;
		}

		c = telnet ? telnet_parse((uint8_t)c) : (uint8_t)c;
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
