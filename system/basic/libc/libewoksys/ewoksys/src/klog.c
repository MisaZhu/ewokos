
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/mstr.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUF_SIZE 128
static char _buf[BUF_SIZE+1];
static int32_t _klog_fd = -1;

void kout(const char *str) {
	syscall2(SYS_KPRINT, (ewokos_addr_t)str, (ewokos_addr_t)strlen(str));
}

void klog(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vsnprintf(_buf, sizeof(_buf), format, ap);
	va_end(ap);

	const char* klog_dev = getenv("KLOG_DEV");
	if(klog_dev == NULL || klog_dev[0] == 0)
		klog_dev = getenv("STDERR_DEV");

	if(_klog_fd <= 0 && klog_dev != NULL) {
		_klog_fd = open(klog_dev, O_WRONLY);
	}

	if(_klog_fd > 0) {
		write(_klog_fd, _buf, strlen(_buf));
	}
	syscall2(SYS_KPRINT, (ewokos_addr_t)_buf, (ewokos_addr_t)strlen(_buf));
}

#ifdef __cplusplus 
}
#endif

