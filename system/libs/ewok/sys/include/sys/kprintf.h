#ifndef KPRINTF_H
#define KPRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

void kprintf(bool tty_only, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
