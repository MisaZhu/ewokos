#include <vprintf.h>
#include <unistd.h>
#include <ewoksys/mstr.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUF_SIZE 128
static char _buf[BUF_SIZE+1];
static int32_t _buf_index;

void kout(const char *str, int len) {
	syscall2(SYS_KPRINT, (int32_t)str, (int32_t)len);
}

void klog(const char *format, ...) {
	va_list ap;
	_buf_index = 0;
	va_start(ap, format);
	vsprintf(_buf, format, ap);
	va_end(ap);
	//if(write(2, buf->cstr, buf->len) <= 0)
		//syscall2(SYS_KPRINT, (int32_t)buf->cstr, (int32_t)buf->len);
	//str_free(buf);
	syscall2(SYS_KPRINT, (int32_t)_buf, strlen(_buf));
}

#ifdef __cplusplus 
}
#endif

