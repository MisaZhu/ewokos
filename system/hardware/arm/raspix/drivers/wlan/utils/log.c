
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <ewoksys/syscall.h>


#define RING_BUF_SIZE 4096

static uint32_t buf_ptr;
static char *ring_buf;
static pthread_mutex_t mutex;

void log_init(void){
    ring_buf = malloc(RING_BUF_SIZE);
    buf_ptr = 0;
    pthread_mutex_init(&mutex, NULL);
}

void brcm_klog(const char *format, ...) {
	va_list ap;
    pthread_mutex_lock(&mutex);
	va_start(ap, format);
	vsnprintf(ring_buf, RING_BUF_SIZE, format, ap);
	va_end(ap);
    pthread_mutex_unlock(&mutex);
	syscall2(SYS_KPRINT, (int32_t)ring_buf, strlen(ring_buf));
}