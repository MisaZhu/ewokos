#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

/*
 * Core of the BSD err/warn family. Prints "fmt: ..." to stderr, appending
 * ": <strerror(errno)>" when use_errno is set, then a newline. errno is
 * preserved across the formatting so the caller's value is reported.
 */
static void ewarn(int use_errno, const char *fmt, va_list ap) {
    int saved = errno;

    if (fmt != NULL) {
        vfprintf(stderr, fmt, ap);
        if (use_errno)
            fprintf(stderr, ": %s", strerror(saved));
    } else if (use_errno) {
        fputs(strerror(saved), stderr);
    }
    fputc('\n', stderr);
    fflush(stderr);

    errno = saved;
}

void vwarnx(const char *fmt, va_list ap) {
    ewarn(0, fmt, ap);
}

void vwarn(const char *fmt, va_list ap) {
    ewarn(1, fmt, ap);
}

void verrx(int eval, const char *fmt, va_list ap) {
    ewarn(0, fmt, ap);
    exit(eval);
}

void verr(int eval, const char *fmt, va_list ap) {
    ewarn(1, fmt, ap);
    exit(eval);
}

void warnx(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ewarn(0, fmt, ap);
    va_end(ap);
}

void warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ewarn(1, fmt, ap);
    va_end(ap);
}

void errx(int eval, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ewarn(0, fmt, ap);
    va_end(ap);
    exit(eval);
}

void err(int eval, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ewarn(1, fmt, ap);
    va_end(ap);
    exit(eval);
}
