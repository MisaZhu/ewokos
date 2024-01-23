#include <stdio.h>
#include <vprintf.h>
#include <unistd.h>
#include <ewoksys/mstr.h>

static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

void dprintf(int fd, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	str_t* buf = str_new("");
	vprintf(outc, buf, format, ap);
	va_end(ap);
	write(fd, buf->cstr, buf->len);
	str_free(buf);
}

