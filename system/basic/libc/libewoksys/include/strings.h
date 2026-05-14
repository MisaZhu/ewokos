#ifndef EWOKOS_LIBC_STRINGS_H
#define EWOKOS_LIBC_STRINGS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);
void bzero(void *s, size_t n);

#ifdef __cplusplus
}
#endif

#endif
