#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

#define BUFFER_MAX_LENGTH 256

void putch(int c);
int getch();
void putstr(const char* s);

int printf_base(void (*putch)(int), const char *format, ...);
int snprintf(char *target, int size, const char *format, ...);
int vsnprintf(char *target, int size, const char *format, va_list ap);

#define printf(...) printf_base(putch, ##__VA_ARGS__)

#endif
