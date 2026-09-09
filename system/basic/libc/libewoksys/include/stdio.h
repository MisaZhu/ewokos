#ifndef EWOKOS_LIBC_STDIO_H
#define EWOKOS_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>

#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/*
 * BUFSIZ: the size of the buffer ISO C says a stream uses by default, and the
 * value setvbuf() accepts as `size` when the caller passes NULL.  It was simply
 * missing, which is invisible until a caller names it - HarfBuzz does, in
 * hb_blob_create_from_file_or_fail(), where it sizes the staging buffer for
 * reading a font file:
 *
 *     hb-blob.cc:752: error: 'BUFSIZ' was not declared in this scope
 *
 * That is on the path to QtGui's text engine, so the gap stopped the whole
 * shaper rather than one optional feature.  1024 is the conventional value and
 * matches what every other libc in this family uses; nothing here depends on a
 * particular number, only on it existing and being large enough to make
 * buffered reads worthwhile.
 */
#define BUFSIZ 1024

/*
 * The three setvbuf() buffering modes.  Only _IONBF existed, which left the set
 * half-finished: code asking for full or line buffering could not even name
 * what it wanted.
 *
 * The specific numbers are not load-bearing.  setvbuf() in src/stdio/setvbuf.c
 * discards both `mode` and `size` and delegates to setbuf(), so buffering
 * behaviour does not actually vary by mode on this target - the values only have
 * to be distinct from each other so that a caller's comparisons and a switch on
 * mode behave sanely.  _IONBF keeps its existing 0 rather than being renumbered
 * to glibc's ordering, because it is already published and something may
 * already depend on it; the two new ones follow from there.
 */
#define _IONBF 0
#define _IOFBF 1
#define _IOLBF 2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FILE {
	int fd;
	int eof;
	int has_unget;
	unsigned char unget_ch;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int fscanf(FILE *stream, const char *format, ...);
int vfscanf(FILE *stream, const char *format, va_list ap);
int sscanf(const char *str, const char *format, ...);
int vsscanf(const char *str, const char *format, va_list ap);
int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
int asprintf(char **strp, const char *format, ...);
int vasprintf(char **strp, const char *format, va_list ap);
int puts(const char *s);
int fputs(const char *s, FILE *stream);
int getchar(void);
int putchar(int c);
int putc(int c, FILE *stream);
int fputc(int c, FILE *stream);
int fgetc(FILE *stream);
int ungetc(int c, FILE *stream);
int feof(FILE *stream);
void rewind(FILE *stream);
void perror(const char *s);
FILE *fopen(const char *path, const char *mode);
FILE *fdopen(int fd, const char *mode);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);
FILE *tmpfile(void);
int fileno(FILE *stream);
char *fgets(char *str, int size, FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);
int fflush(FILE *stream);
void setbuf(FILE *stream, char *buf);
int setvbuf(FILE *stream, char *buf, int mode, size_t size);

FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);

#ifdef __cplusplus
}
#endif

#endif
