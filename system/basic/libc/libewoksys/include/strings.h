#ifndef EWOKOS_LIBC_STRINGS_H
#define EWOKOS_LIBC_STRINGS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
void bzero(void *s, size_t n);
void bcopy(const void *src, void *dest, size_t n);
int bcmp(const void *s1, const void *s2, size_t n);
char* strsep(char** stringp, const char* delim);
int ffs(int i);
int ffsl(long i);
int ffsll(long long i);

#ifdef __cplusplus
}
#endif

#endif
