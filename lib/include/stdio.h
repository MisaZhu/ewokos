#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

void putch(int c);
int getch();
void putstr(const char* s);

int printf_base(void (*putch)(int), const char *format, ...);
int sprintf(char *target, const char *format, ...);
int vsprintf(char *target, const char *format, va_list ap);

#define printf(...) printf_base(putch, ##__VA_ARGS__)

#endif
