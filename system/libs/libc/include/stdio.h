#ifndef STDIO_H
#define STDIO_H

#include <fcntl.h>

void printf(const char *format, ...);
void kprintf(const char *format, ...);
void dprintf(int fd, const char *format, ...);

int getch(void);
void putch(int c);

typedef struct {
	int fd;
	int oflags;
} FILE;

FILE*    fopen(const char* fname, const char* mode);
uint32_t fread(void* ptr, uint32_t size, uint32_t nmemb, FILE* fp);
uint32_t fwrite(const void* ptr, uint32_t size, uint32_t nmemb, FILE* fp);
int      fseek(FILE* fp, int offset, int whence);
void     fclose(FILE* fp);

#endif
