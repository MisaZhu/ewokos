#ifndef EWOKOS_LIBC_STRING_H
#define EWOKOS_LIBC_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);

char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strtok(char *str, const char *delim);
const char *strstr(const char *haystack, const char *needle);
char *strdup(const char *s);
char *strndup(const char *s, size_t n);
char *strerror(int errnum);

#ifdef __cplusplus
}
#endif

#endif
