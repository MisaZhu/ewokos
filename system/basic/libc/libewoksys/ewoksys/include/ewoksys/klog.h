#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * logd dev_cntl command: write data straight into the log ring buffer,
 * bypassing the /dev/log open()+write() file path. The caller packs the bytes
 * as a single proto string; logd appends them and wakes any pending readers.
 */
#define CTRL_WRITE 1

void kout(const char *str, uint32_t len);
void klog(const char *format, ...);
void flog(int fd, const char *format, ...);
void slog(const char *format, ...);
void sout(const char *str, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
