#include "printk.h"
#include "vsprintf.h"
#include "dev/uart.h"

#define BUFFER_MAX_LENGTH 256

void printk(const char *format, ...) {
	char buffer[BUFFER_MAX_LENGTH] = {0};
	va_list ap;

	va_start(ap, format);
	vsnprintf(buffer, BUFFER_MAX_LENGTH-1, format, ap);
	va_end(ap);
	uartPuts(buffer);
}
