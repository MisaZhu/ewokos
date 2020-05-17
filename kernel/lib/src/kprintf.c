#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "dev/actled.h"
#include "kstring.h"
#include "kernel/system.h"
#include <stddef.h>

#ifdef FRAMEBUFFER
#include "dev/framebuffer.h"
#endif

void uart_out(const char* s) {
	uart_write(s, strlen(s));
}

#define PRINTF_BUF_MAX 128
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

void flush_actled(void) {
	act_led(1);
	_delay(1000000);
	act_led(0);
	_delay(1000000);
}

void printf(const char *format, ...) {
	act_led(1);
	_delay_msec(10);

	va_list ap;
	va_start(ap, format);
	_len = 0;
	v_printf(outc, NULL, format, ap);
	uart_out(_buf);
	act_led(0);
}

