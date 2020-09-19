#include <stdio.h>
#include <vprintf.h>
#include <unistd.h>
#include <sys/mstr.h>

static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

void printf(const char *format, ...) {
	va_list ap;
	str_t* buf = str_new("");
	va_start(ap, format);
	v_printf(outc, buf, format, ap);
	va_end(ap);
	write(1, buf->cstr, buf->len);
	str_free(buf);
}

