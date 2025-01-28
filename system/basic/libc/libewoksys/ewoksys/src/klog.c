
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <ewoksys/mstr.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUF_SIZE 128
static char _buf[BUF_SIZE+1];
static int32_t _klog_fd = -1;

void kout(const char *str) {
	syscall2(SYS_KPRINT, (int32_t)str, (int32_t)strlen(str));
}

void klog(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vsnprintf(_buf, sizeof(_buf), format, ap);
	va_end(ap);

	if(_klog_fd <= 0) {
		_klog_fd = open("/dev/klog", O_WRONLY);
	}

	if(_klog_fd > 0) {
		write(_klog_fd, _buf, strlen(_buf));
	}
	syscall2(SYS_KPRINT, (int32_t)_buf, strlen(_buf));
}

#ifdef __cplusplus 
}
#endif

