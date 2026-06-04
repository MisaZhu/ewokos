#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ewoksys/vfs.h>
#include <ewoksys/core.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <sys/errno.h>

#include <ewoksys/mstr.h>
#include <ewoksys/keydef.h>
#include "shell.h"

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

static int is_telnet_console(void) {
	const char* cid = getenv("CONSOLE_ID");
	return cid != NULL && strcmp(cid, "telnet") == 0;
}

static uint8_t telnet_parse(uint8_t c, const char **event) {
	enum {
		TELNET_STATE_DATA = 0,
		TELNET_STATE_IAC,
		TELNET_STATE_CMD,
		TELNET_STATE_SB,
		TELNET_STATE_SB_DATA,
		TELNET_STATE_SB_IAC,
		TELNET_STATE_CR,
	};
	static int state = TELNET_STATE_DATA;
	static uint8_t verb = 0;
	uint8_t ret = 0;

	if(event != NULL)
		*event = "data";

	switch(state) {
	case TELNET_STATE_DATA:
		if(c == 0xFF) {
			state = TELNET_STATE_IAC;
			if(event != NULL)
				*event = "iac-begin";
		}
		else if(c == '\r') {
			state = TELNET_STATE_CR;
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
		if(c == 0xFF) {
			ret = 0xFF;
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
	case TELNET_STATE_CR:
		if(c == 0) {
			if(event != NULL)
				*event = "cr-nul-tail";
		}
		else if(c == '\n') {
			if(event != NULL)
				*event = "cr-lf-tail";
		}
		else {
			ret = c;
			if(event != NULL)
				*event = "cr-data";
		}
		state = TELNET_STATE_DATA;
		break;
	default:
		state = TELNET_STATE_DATA;
		if(event != NULL)
			*event = "reset";
		break;
	}

	return ret;
}

int32_t cmd_gets(int fd, str_t* buf) {
	str_reset(buf);	
	old_cmd_t* head = NULL;
	old_cmd_t* tail = NULL;
	bool first_up = true;
	bool echo = true;
	bool telnet = (fd == 0) && is_telnet_console();

	while(1) {
		char c, old_c;
		errno = 0;
		if(fd == 0 && !_script_mode) {
			klog("shell cmd_gets before read fd=0\n");
		}
		int i = read(fd, &c, 1);
		if(fd == 0 && !_script_mode) {
			klog("shell cmd_gets after read fd=0 i=%d errno=%d c=%d\n", i, errno, (int)(unsigned char)c);
		}
		if(i == 0) {
			/*
			 * Telnet/console-like stdin can occasionally surface an empty read
			 * transiently even though the session is still alive.
			 */
			if(fd == 0 && !_script_mode) {
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
			if(fd == 0 && !_script_mode) {
				klog("shell cmd_gets parsed fd=0 c=%d event=%s\n",
						(int)(unsigned char)c,
						telnet_event == NULL ? "?" : telnet_event);
			}
			if(c == 0)
				continue;
		}
		if(c == 0 || i < 0) {
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
				if(fd == 0 && !_script_mode) {
					klog("shell cmd_gets newline: len=%d buf='%s'\n", buf->len, buf->cstr);
				}
				break;
			}
			if(c > 27)
				str_addc(buf, c);
		}
	}
	str_addc(buf, 0);
	if(fd == 0 && !_script_mode) {
		klog("shell cmd_gets return: len=%d buf='%s'\n", buf->len, buf->cstr);
	}
	return 0;
}
