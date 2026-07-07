#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ewoksys/vfs.h>
#include <ewoksys/core.h>
#include <ewoksys/ipc.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <sys/errno.h>

#include <ewoksys/mstr.h>
#include <ewoksys/keydef.h>
#include <poll.h>
#include "shell.h"

#define TELNET_IAC  255
#define TELNET_DONT 254
#define TELNET_DO   253
#define TELNET_WONT 252
#define TELNET_WILL 251

#define TELNET_OPT_ECHO 1
#define TELNET_OPT_SUPPRESS_GA 3
#define TELNET_OPT_LINEMODE 34

void add_history(const char* cmd) {
	if(_history != NULL && strcmp(cmd, _history->cmd->cstr) == 0)
		return;

	old_cmd_t* oc = (old_cmd_t*)malloc(sizeof(old_cmd_t));	
	oc->cmd = str_new(cmd);
	oc->prev = NULL;
	oc->next = _history;
	if(_history != NULL)
		_history->prev = oc;
	else
		_history_tail = oc;
	_history = oc;
}

void free_history(void) {
	old_cmd_t* oc = _history_tail;
	while(oc != NULL) {
		old_cmd_t* prev = oc->prev;
		str_free(oc->cmd);
		free(oc);
		oc = prev;
	}
}

static void clear_buf(str_t* buf) {
	if(buf->len == 0)
		return;

	uint32_t i = 0;
	while(i < buf->len) {
		buf->cstr[i] = CONSOLE_LEFT; 
		i++;
	}
	if(buf->len > 0)
		puts(buf->cstr);
	buf->len = 0;
}

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

static uint8_t telnet_parse(uint8_t c, const char **event) {
	enum {
		TELNET_STATE_DATA = 0,
		TELNET_STATE_IAC,
		TELNET_STATE_CMD,
		TELNET_STATE_SB,
		TELNET_STATE_SB_DATA,
		TELNET_STATE_SB_IAC,
	};
	static int state = TELNET_STATE_DATA;
	static int pending_cr_tail = 0;
	static uint8_t verb = 0;
	uint8_t ret = 0;

	if(event != NULL)
		*event = "data";

	if(state == TELNET_STATE_DATA && pending_cr_tail != 0) {
		pending_cr_tail = 0;
		if(c == 0) {
			if(event != NULL)
				*event = "cr-nul-tail";
			return 0;
		}
		if(c == '\n') {
			if(event != NULL)
				*event = "cr-lf-tail";
			return 0;
		}
	}

	switch(state) {
	case TELNET_STATE_DATA:
		if(c == TELNET_IAC) {
			state = TELNET_STATE_IAC;
			if(event != NULL)
				*event = "iac-begin";
		}
		else if(c == '\r') {
			pending_cr_tail = 1;
			ret = '\n';
			if(event != NULL)
				*event = "cr->newline";
		}
		else {
			ret = c;
			if(event != NULL)
				*event = "data-byte";
		}
		break;
	case TELNET_STATE_IAC:
		if(c == TELNET_IAC) {
			ret = TELNET_IAC;
			state = TELNET_STATE_DATA;
			if(event != NULL)
				*event = "iac-escaped-ff";
		}
		else if(c == 0xFA) { /* SB */
			state = TELNET_STATE_SB;
			if(event != NULL)
				*event = "iac-subneg-begin";
		}
		else if(c == 0xF0) { /* SE */
			state = TELNET_STATE_DATA;
			if(event != NULL)
				*event = "iac-subneg-end";
		}
		else {
			verb = c;
			state = TELNET_STATE_CMD;
			if(event != NULL)
				*event = "iac-verb";
		}
		break;
	case TELNET_STATE_CMD:
		telnet_reply_option(verb, c);
		(void)verb;
		state = TELNET_STATE_DATA;
		if(event != NULL)
			*event = "iac-option";
		break;
	case TELNET_STATE_SB:
		state = TELNET_STATE_SB_DATA;
		if(event != NULL)
			*event = "sb-option";
		break;
	case TELNET_STATE_SB_DATA:
		if(c == 0xFF)
			state = TELNET_STATE_SB_IAC;
		if(event != NULL)
			*event = "sb-data";
		break;
	case TELNET_STATE_SB_IAC:
		if(c == 0xF0) /* SE */
			state = TELNET_STATE_DATA;
		else if(c != 0xFF)
			state = TELNET_STATE_SB_DATA;
		if(event != NULL)
			*event = (c == 0xF0) ? "sb-end" : "sb-iac";
		break;
	default:
		state = TELNET_STATE_DATA;
		if(event != NULL)
			*event = "reset";
		break;
	}

	return ret;
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

int32_t cmd_gets(int fd, str_t* buf) {
	str_reset(buf);	
	old_cmd_t* head = NULL;
	old_cmd_t* tail = NULL;
	bool first_up = true;
	bool telnet = (fd == 0) && is_telnet_console();
	bool echo = true;

	while(1) {
		char c, old_c;
		errno = 0;
		int i = telnet ? telnet_read_raw(fd, &c) : read(fd, &c, 1);
		if(i == 0) {
			/*
			 * Keep local console stdin tolerant of transient empty reads, but a
			 * telnet-backed stdin must treat read(2)==0 as EOF so the remote shell
			 * exits cleanly when the client disconnects.
			 */
			if(fd == 0 && !_script_mode && !telnet) {
				proc_usleep(10000);
				continue;
			}
			return -1;
		}
	 	if(i < 0) {
			if(errno == EAGAIN || errno == EINTR || errno == 0) {
				proc_usleep(10000);
				continue;
			}
			return -1;
		}
		
		if(telnet) {
			const char *telnet_event = NULL;
			c = telnet_parse((uint8_t)c, &telnet_event);
			if(c == 0) {
				continue;
			}
		}
		if(c == 0 || i < 0) {
			proc_usleep(10000);
			continue;
		}

		if (c == KEY_BACKSPACE) {
			if (buf->len > 0) {
				//delete last char
				static const char erase_seq[3] = { CONSOLE_LEFT, ' ', CONSOLE_LEFT };
				(void)write_all_retry(1, erase_seq, sizeof(erase_seq));
				buf->len--;
			}
		}
		else if (c == CONSOLE_LEFT) {
			if (buf->len > 0) {
				//delete last char
				(void)write_all_retry(1, &c, 1);
				buf->len--;
			}
		}
		else if (c == KEY_UP) { //prev command
			if(first_up) {
				head = _history;
				first_up = false;
			}
			else if(head != NULL) {
				head = head->next;
			}

			if(head != NULL) {
				tail = head;
				clear_buf(buf);
				str_cpy(buf, head->cmd->cstr);
				if(buf->len > 0)
					puts(buf->cstr);
			}
		}
		else if (c == KEY_DOWN) { //next command
			if(tail != NULL)
				tail = tail->prev;
			clear_buf(buf);

			if(tail != NULL) {
				head = tail;
				str_cpy(buf, tail->cmd->cstr);
				if(buf->len > 0)
					puts(buf->cstr);
			}
			else {
				head = _history;
				first_up = true;
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

			if(buf->len == 0 && (c == '@' || c == '#'))
				echo = false;
			if(echo && !_script_mode)
				putch(c);
			if(c == '\n') {
				break;
			}
			if(c > 27)
				str_addc(buf, c);
		}
	}
	str_addc(buf, 0);
	return 0;
}
