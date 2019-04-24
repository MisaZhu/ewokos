#ifndef KSTRING_H
#define KSTRING_H

#include "types.h"

/* copy functions */
void *memcpy(void *target, const void *source, uint32_t n);
char *strcpy(char *target, const char *source);
uint32_t strncpy(char *target, const char *source, uint32_t n);

/* compare functions */
int32_t strcmp(const char *s1, const char *s2);
int32_t strncmp(const char *s1, const char *s2, uint32_t n);

/* search and tokenization */
char *strchr(const char *str, int32_t character);
char *strtok(char *str, const char *delimiters);

/* other functions */
void *memset(void *target, int32_t c, uint32_t len);
uint32_t strlen(const char *str);

#endif
