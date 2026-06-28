#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "kstring.h"
#include "kernel/system.h"
#include "kernel/kernel.h"
#include <kernel/core.h>
#include <stddef.h>

static int32_t _kout_spin = 0;

void kout(const char* s, uint32_t len) {
	if(s == NULL || len == 0)
		return;

	uint32_t irq_state = __int_off();
#ifdef KERNEL_SMP
	mcore_lock(&_kout_spin);
#endif
	uart_write(s, len);
#ifdef KERNEL_SMP
	mcore_unlock(&_kout_spin);
#endif
	__int_on(irq_state);
}

void kout_str(const char* s) {
	kout(s, strlen(s));
}

#define PRINTF_BUF_MAX 1024
static uint32_t _len = 0;
static char _buf[PRINTF_BUF_MAX+1];
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
	memset(_buf, 0, PRINTF_BUF_MAX+1);
	v_printf(outc, NULL, format, ap);
	kout(_buf, strlen(_buf));
	va_end(ap);
}
