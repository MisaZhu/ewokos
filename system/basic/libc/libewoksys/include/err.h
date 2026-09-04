#ifndef EWOKOS_ERR_H
#define EWOKOS_ERR_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BSD err/warn family. Messages are written to stderr; the err()/warn()
 * forms append a description of the current errno. The err*() forms then
 * terminate the process with the given exit status, the warn*() forms do
 * not. The v*() forms take a va_list instead of a variable argument list.
 */
void err(int eval, const char *fmt, ...);
void errx(int eval, const char *fmt, ...);
void warn(const char *fmt, ...);
void warnx(const char *fmt, ...);
void verr(int eval, const char *fmt, va_list ap);
void verrx(int eval, const char *fmt, va_list ap);
void vwarn(const char *fmt, va_list ap);
void vwarnx(const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif

#endif
