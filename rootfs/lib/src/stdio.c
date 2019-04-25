#include <syscall.h>
#include <kstring.h>
#include <vprintf.h>
#include <device.h>
#include <unistd.h>
#include <device.h>
#include <stdio.h>

int32_t _stdin =-1;
int32_t _stdout = -1;

#define STDOUT_BUF_SIZE 128
static char _out_buffer[STDOUT_BUF_SIZE];
static int32_t _out_size = 0;

void init_stdio() {
	_stdin = _stdout = open("/dev/console0", 0);
	//_stdin = _stdout = open("/dev/tty0", 0);
	_out_size = 0;
}

static void stdout_flush() {
	if(_stdout < 0)
		syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_UART, 0), (int32_t)_out_buffer, _out_size);
	else
		write(_stdout, _out_buffer, _out_size);
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
		if(_stdin < 0)
			res = syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_UART, 0), (int32_t)buf, 1);
		else
			res = read(_stdin, buf, 1);
		if(res > 0)
			break;
	}
	return (int)buf[0];
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
