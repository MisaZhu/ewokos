#include <syscall.h>
#include <kstring.h>
#include <vprintf.h>

void putch(int c) {
	syscall1(SYSCALL_UART_PUTCH, c);
}

int getch() {
	return syscall0(SYSCALL_UART_GETCH);
}

static void outc(char c, void* p) {
	(void)p;
	putch(c);
}

/*
 * printf_base formats and prints the given data. See vsprintf() for the format
 * flags currently supported.
 */
void printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	v_printf(outc, NULL, format, ap);
	va_end(ap);
}
