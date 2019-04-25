#ifndef STDIO_H
#define STDIO_H

#include <vprintf.h>

extern int32_t _stdin;
extern int32_t _stdout;

void init_stdio();
void putch(int c);
int getch();
void printf(const char *format, ...);

#endif
