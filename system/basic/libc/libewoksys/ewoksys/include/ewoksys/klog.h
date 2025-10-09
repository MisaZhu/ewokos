#ifndef KPRINTF_H
#define KPRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

void kout(const char *str);
void klog(const char *format, ...);
void slog(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
