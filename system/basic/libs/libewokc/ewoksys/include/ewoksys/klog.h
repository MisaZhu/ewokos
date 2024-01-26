#ifndef KPRINTF_H
#define KPRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

void kout(const char *str, int len);
void klog(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
