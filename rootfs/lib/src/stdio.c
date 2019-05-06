#include <syscall.h>
#include <kstring.h>
#include <vprintf.h>
#include <device.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <vfs/fs.h>

#define STDOUT_BUF_SIZE 128
static char _out_buffer[STDOUT_BUF_SIZE];
static int32_t _out_size = 0;

static void stdout_flush() {
	fs_info_t info;
	bool uart = false;
	if(fs_info(_stdout, &info) != 0)
		uart = true;

	const char* p = _out_buffer;
	while(_out_size > 0) {
		int32_t res;
		if(uart) 
			res = syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_UART, 0), (int32_t)_out_buffer, _out_size);
		else
			res = write(_stdout, p, _out_size);

		if(res > 0) {
			p += res;	
			_out_size -= res;
		}
		else if(errno != EAGAIN)
			break;
	}
	_out_size = 0;
}

void init_stdout_buffer() {
	_out_size = 0;
}

void putch(int c) {
	_out_buffer[_out_size] = c;
	_out_size++;
	stdout_flush();
}

int getch() {
	char buf[1];
	buf[0] = 0;

	int32_t res = -1;
	while(true) {
		res = read(_stdin, buf, 1);
		//res = syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_UART, 0), (int32_t)buf, 1);
		if(res < 0 && errno == EAGAIN) {
			sleep(0);
			continue;
		}
		break;
	}
	if(res > 0)
		return (int)buf[0];
	return 0;
}

static void outc(char c, void* p) {
	(void)p;
	_out_buffer[_out_size] = c;
	_out_size++;
	if(_out_size >= STDOUT_BUF_SIZE || c == '\n' || c == '\r') {
		stdout_flush();
	}
}

/*
 * printf_base formats and prints the given data. See vsprintf() for the format
 * flags currently supported.
 */
void printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	v_printf(outc, NULL, format, ap);
	stdout_flush();
	va_end(ap);
}
