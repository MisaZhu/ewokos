#include <string.h>

char *stpcpy(char *dest, const char *src) {
    while (*src != '\0') {
        *dest++ = *src++;
    }
    *dest = '\0';
    return dest;
}

char *stpncpy(char *dest, const char *src, size_t n) {
    size_t srclen = strnlen(src, n);

    for (size_t i = 0; i < srclen; i++) {
        dest[i] = src[i];
    }
    for (size_t i = srclen; i < n; i++) {
        dest[i] = '\0';
    }
    /* points at the NUL terminator, or dest+n when src was truncated */
    return dest + srclen;
}
