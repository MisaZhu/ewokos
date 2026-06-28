
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <ewoksys/mstr.h>
#include <ewoksys/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUF_SIZE 512
static int32_t _slog_fd = -1;
static int32_t _klog_fd = -1;
static pthread_mutex_t _log_lock = 0;

static inline void log_lock(void) {
	if(_log_lock == 0) {
		pthread_mutex_init(&_log_lock, NULL);
	}
	pthread_mutex_lock(&_log_lock);
}

static inline void log_unlock(void) {
	if(_log_lock != 0) {
		pthread_mutex_unlock(&_log_lock);
	}
}

void kout(const char *str, uint32_t len) {
	if(str == NULL || len == 0)
		return;
	syscall2(SYS_KPRINT, (ewokos_addr_t)str, (ewokos_addr_t)len);
}

void klog(const char *format, ...) {
	char buf[BUF_SIZE+1];
	log_lock();
	va_list ap;
	va_start(ap, format);
	memset(buf, 0, sizeof(buf));
	int len = vsnprintf(buf, BUF_SIZE, format, ap);
	va_end(ap);

	if(len < 0) {
		log_unlock();
		return;
	}
	if(len > BUF_SIZE) {
		len = BUF_SIZE;
	}
	syscall2(SYS_KPRINT, (ewokos_addr_t)buf, (ewokos_addr_t)len);
	log_unlock();

	/*const char* log_dev = "/dev/klog";
	if(_klog_fd <= 0 && log_dev != NULL) {
		_klog_fd = open(log_dev, O_WRONLY);
	}

	if(_klog_fd > 0) {
		write(_klog_fd, _buf, strlen(_buf));
	}
	*/
}

void sout(const char *str, uint32_t len) {
	if(str == NULL || len == 0)
		return;
	log_lock();
	const char* log_dev = "/dev/log";
	if(_slog_fd <= 0 && log_dev != NULL) {
		_slog_fd = open(log_dev, O_WRONLY);
	}

	if(_slog_fd > 0) {
		write(_slog_fd, str, len);
	}
	else {
		kout(str, len);
	}
	log_unlock();
}

void slog(const char *format, ...) {
	char buf[BUF_SIZE+1];
	va_list ap;
	va_start(ap, format);
	memset(buf, 0, sizeof(buf));
	vsnprintf(buf, BUF_SIZE, format, ap);
	va_end(ap);
	sout(buf, strlen(buf));
}

#ifdef __cplusplus 
}
#endif
