#ifndef STRING_H
#define STRING_H

#include <types.h>

/* copy functions */
void *memcpy(void *target, const void *source, size_t n);
char *strcpy(char *target, const char *source);
size_t strlcpy(char *target, const char *source, size_t n);

/* compare functions */
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

/* search and tokenization */
char *strchr(const char *str, int character);
char *strtok(char *str, const char *delimiters);

/* other functions */
void *memset(void *target, int c, size_t len);
size_t strlen(const char *str);

#endif
