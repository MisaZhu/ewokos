
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

#define BUF_SIZE 256
static char _buf[BUF_SIZE+1];
static int32_t _slog_fd = -1;
static int32_t _klog_fd = -1;

void kout(const char *str) {
	syscall2(SYS_KPRINT, (ewokos_addr_t)str, (ewokos_addr_t)strlen(str));
}

void klog(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vsnprintf(_buf, sizeof(_buf), format, ap);
	va_end(ap);

	syscall2(SYS_KPRINT, (ewokos_addr_t)_buf, (ewokos_addr_t)strlen(_buf));

	/*const char* log_dev = "/dev/klog";
	if(_klog_fd <= 0 && log_dev != NULL) {
		_klog_fd = open(log_dev, O_WRONLY);
	}

	if(_klog_fd > 0) {
		write(_klog_fd, _buf, strlen(_buf));
	}
	*/
}

void slog(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vsnprintf(_buf, sizeof(_buf), format, ap);
	va_end(ap);

	const char* log_dev = "/dev/log";
	if(_slog_fd <= 0 && log_dev != NULL) {
		_slog_fd = open(log_dev, O_WRONLY);
	}

	if(_slog_fd > 0) {
		write(_slog_fd, _buf, strlen(_buf));
	}
	else {
		syscall2(SYS_KPRINT, (ewokos_addr_t)_buf, (ewokos_addr_t)strlen(_buf));
	}
}

#ifdef __cplusplus 
}
#endif

