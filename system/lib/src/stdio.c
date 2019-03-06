#include <syscall.h>
#include <kstring.h>
#include <vsprintf.h>

#define BUFFER_MAX_LENGTH 256

void putch(int c) {
	syscall1(SYSCALL_UART_PUTCH, c);
}

int getch() {
	return syscall0(SYSCALL_UART_GETCH);
}

/*
 * printf_base formats and prints the given data. See vsprintf() for the format
 * flags currently supported.
 */
int printf(const char *format, ...) {
	int length = 0;
	char buffer[BUFFER_MAX_LENGTH+1] = {0};
	int i = 0;
	va_list ap;

	va_start(ap, format);
	length = vsnprintf(buffer, BUFFER_MAX_LENGTH, format, ap);
	va_end(ap);

	while (i < length && buffer[i] != '\0') {
		putch(buffer[i]);
		i++;
	}
	return length;
}
