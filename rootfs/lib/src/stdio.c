#include <syscall.h>
#include <kstring.h>
#include <vprintf.h>
#include <device.h>
#include <unistd.h>

void putch(int c) {
	char buf[1];
	buf[0] = (char)c;
	syscall3(SYSCALL_DEV_CHAR_WRITE, dev_typeid(DEV_UART, 0), (int32_t)buf, 1);
}

int getch() {
	char buf[1];
	while(true) {
		if(syscall3(SYSCALL_DEV_CHAR_READ, dev_typeid(DEV_UART, 0), (int32_t)buf, 1) > 0)
			break;
	}
	return (int)buf[0];
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
