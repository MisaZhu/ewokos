#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

void putch(int c);
int getch();

int printf(const char *format, ...);

#endif
