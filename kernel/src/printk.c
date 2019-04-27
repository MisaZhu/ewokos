#include "printk.h"
#include "vprintf.h"
#include "dev/basic_dev.h"

static void outc(char c, void* p) {
	(void)p;
	dev_char_write(dev_typeid(DEV_UART, 0), &c, 1);
}

void printk(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	v_printf(outc, NULL, format, ap);
	va_end(ap);
}
