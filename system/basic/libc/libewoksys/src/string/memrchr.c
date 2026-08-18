#include <string.h>

void *memrchr(const void *s, int c, size_t n) {
    const unsigned char *p = (const unsigned char *)s;

    while (n > 0) {
        if (p[n - 1] == (unsigned char)c)
            return (void *)(p + n - 1);
        n--;
    }
    return NULL;
}

void *memccpy(void *dest, const void *src, int c, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
        if (s[i] == (unsigned char)c)
            return d + i + 1;
    }
    return NULL;
}
