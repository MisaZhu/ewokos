#include <stdio.h>
#include <vprintf.h>
#include <unistd.h>
#include <ewoksys/mstr.h>
#include <fcntl.h>

static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

void fprintf(FILE* fp, const char *format, ...) {
	if(fp == NULL || ((fp->oflags & O_WRONLY) == 0 && (fp->oflags & O_RDWR) == 0))
		return;

	va_list ap;
	va_start(ap, format);
	str_t* buf = str_new("");
	v_printf(outc, buf, format, ap);
	va_end(ap);
	write(fp->fd, buf->cstr, buf->len);
	str_free(buf);
}

