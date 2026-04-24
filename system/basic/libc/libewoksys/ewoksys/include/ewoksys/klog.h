#ifndef KPRINTF_H
#define KPRINTF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void kout(const char *str, uint32_t len);
void klog(const char *format, ...);
void slog(const char *format, ...);
void sout(const char *str, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
