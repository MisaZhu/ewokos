#ifndef __LOG_H__
#define __LOG_H__

#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <ewoksys/kernel_tic.h>

#define usleep	        proc_usleep
#define get_timer(x)    (kernel_tic_ms(0) - (x))

void log_init(void);
void brcm_log(const char *format, ...);
char* brcm_get_log(void);
#endif
