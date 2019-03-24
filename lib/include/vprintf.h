#ifndef VPRINTF_H
#define VPRINTF_H

#include <stdarg.h>

typedef void (*outc_func_t)(char c, void* p);

void v_printf(outc_func_t outc, void* p, const char* format, va_list ap);
int snprintf(char *target, int size, const char *format, ...);

#endif
