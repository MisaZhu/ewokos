
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <ewoksys/syscall.h>
#include <ewoksys/charbuf.h>


#define RING_BUF_SIZE 4096
#define TEMP_BUF_SIZE 128

static charbuf_t *LogBuf;
static char *temp_buf;
static pthread_mutex_t mutex;

void log_init(void){
    LogBuf = charbuf_new(RING_BUF_SIZE);
    temp_buf = malloc(TEMP_BUF_SIZE);
    pthread_mutex_init(&mutex, NULL);
}

void brcm_log(const char *format, ...) {
	va_list ap;
    pthread_mutex_lock(&mutex);
    uint64_t ts = kernel_tic_ms(0);
    int len = sprintf(temp_buf, "%d:", ts);
	va_start(ap, format);
	vsnprintf(temp_buf + len, TEMP_BUF_SIZE, format, ap);
	va_end(ap);

    int i = 0;
    while(temp_buf[i]!= '\0'){
        charbuf_push(LogBuf, temp_buf[i], true);
        i++;
    }
    pthread_mutex_unlock(&mutex);
	//syscall2(SYS_KPRINT, (ewokos_addr_t)ring_buf, (ewokos_addr_t)strlen(ring_buf));
}

char* brcm_get_log(void){
    char* buf = malloc(RING_BUF_SIZE);
   int ret, i = 0;

    while(true){
        ret =  charbuf_pop(LogBuf, &buf[i]);
        if(ret != 0)
            break;
        i++;
    };
    buf[i] = '\0';
    return buf;
}