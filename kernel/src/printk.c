#include "printk.h"
#include "vprintf.h"
#include "dev/uart.h"

static void outc(char c, void* p) {
	(void)p;
	uart_putch(c);
}

void printk(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	v_printf(outc, NULL, format, ap);
	va_end(ap);
}
