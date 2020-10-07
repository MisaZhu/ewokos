#include <vprintf.h>
#include <sys/mstr.h>
#include <sys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

static void outc(char c, void* p) {
	str_t* buf = (str_t*)p;
	str_addc(buf, c);
}

void klog(const char *format, ...) {
	va_list ap;
	str_t* buf = str_new("");
	va_start(ap, format);
	v_printf(outc, buf, format, ap);
	va_end(ap);
	syscall2(SYS_KPRINT, (int32_t)buf->cstr, (int32_t)buf->len);
	write(2, buf->cstr, buf->len);
	str_free(buf);
}

#ifdef __cplusplus 
}
#endif

