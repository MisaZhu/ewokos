#ifndef STDIO_H
#define STDIO_H

#include <sys/ewokdef.h>

#ifdef __cplusplus
extern "C" {
#endif


void printf(const char *format, ...);
void dprintf(int fd, const char *format, ...);
int snprintf(char *target, int size, const char *format, ...);

int  getch(void);
void putch(int c);
int  puts(const char* s);

typedef struct {
	int fd;
	int oflags;
} FILE;

FILE*    fopen(const char* fname, const char* mode);
uint32_t fread(void* ptr, uint32_t size, uint32_t nmemb, FILE* fp);
uint32_t fwrite(const void* ptr, uint32_t size, uint32_t nmemb, FILE* fp);
int      fseek(FILE* fp, int offset, int whence);
void     fclose(FILE* fp);
void     rewind(FILE* fp);
int      ftell(FILE* fp);
void     fprintf(FILE* fp, const char *format, ...);

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

#ifdef __cplusplus
}
#endif

#endif
