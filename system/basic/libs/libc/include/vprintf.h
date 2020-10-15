#ifndef VPRINTF_H
#define VPRINTF_H

#include <sys/ewokdef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*outc_func_t)(char c, void* p);

void v_printf(outc_func_t outc, void* p, const char* format, va_list ap);

#ifdef __cplusplus
}
#endif

#endif
