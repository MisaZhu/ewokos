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
#include <utils/telnet_console.h>
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

int32_t cmd_gets(int fd, str_t* buf) {
	str_reset(buf);	
	old_cmd_t* head = NULL;
	old_cmd_t* tail = NULL;
	bool first_up = true;
	bool telnet = (fd == 0) && telnet_console_is_active();
	bool echo = true;

	while(1) {
		char c, old_c;
		errno = 0;
		int i = telnet ? telnet_console_read(fd, &_telnet_console, &c) : read(fd, &c, 1);
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
			c = telnet_console_parse(&_telnet_console, (uint8_t)c);
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
