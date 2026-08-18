#include <string.h>

static int lower_eq(char a, char b) {
    if (a >= 'A' && a <= 'Z')
        a += 'a' - 'A';
    if (b >= 'A' && b <= 'Z')
        b += 'a' - 'A';
    return a == b;
}

char *strcasestr(const char *haystack, const char *needle) {
    size_t nlen;

    if (needle == NULL || *needle == '\0')
        return (char *)haystack;

    nlen = strlen(needle);
    for (const char *p = haystack; *p != '\0'; p++) {
        size_t i;
        for (i = 0; i < nlen; i++) {
            if (p[i] == '\0' || !lower_eq(p[i], needle[i]))
                break;
        }
        if (i == nlen)
            return (char *)p;
    }
    return NULL;
}
