#ifndef STDIO_H
#define STDIO_H

#include <vprintf.h>

static const int32_t _stdin = 0;
static const int32_t _stdout = 1;

void init_stdout_buffer();
void putch(int c);
int getch();
void printf(const char *format, ...);

#endif
