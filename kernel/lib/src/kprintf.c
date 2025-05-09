#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "kstring.h"
#include "kernel/system.h"
#include "kernel/kernel.h"
#include <stddef.h>

void kout(const char* s) {
	uart_write(s, strlen(s));
}

#define PRINTF_BUF_MAX 1024
static uint32_t _len = 0;
static char _buf[PRINTF_BUF_MAX];
static void outc(char c, void* p) {
	(void)p;
	if((_len+1) >= PRINTF_BUF_MAX)
		return;
	_buf[_len] = c;
	_len++;
	_buf[_len] = 0;
}

void printf(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	_len = 0;
	v_printf(outc, NULL, format, ap);
	kout(_buf);
	va_end(ap);
}
