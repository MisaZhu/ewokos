#ifndef VSPRINTF_H
#define VSPRINTF_H

#include <stdarg.h>

int snprintf(char *target, int size, const char *format, ...);
int vsnprintf(char *target, int size, const char *format, va_list ap);

#endif
