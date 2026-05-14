#ifndef EWOKOS_LIBC_STDIO_H
#define EWOKOS_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FILE {
	int fd;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int sscanf(const char *str, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int puts(const char *s);
int getchar(void);
int putchar(int c);
int putc(int c, FILE *stream);
void perror(const char *s);
FILE *fopen(const char *path, const char *mode);
FILE *fdopen(int fd, const char *mode);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int ferror(FILE *stream);
int fflush(FILE *stream);
void setbuf(FILE *stream, char *buf);

#ifdef __cplusplus
}
#endif

#endif
