#include <strings.h>
#include <string.h>

void bcopy(const void *src, void *dest, size_t n) {
    memmove(dest, src, n);
}

int bcmp(const void *s1, const void *s2, size_t n) {
    return memcmp(s1, s2, n);
}

int ffs(int i) {
    unsigned int v = (unsigned int)i;
    int pos = 1;

    if (v == 0)
        return 0;
    while ((v & 1u) == 0) {
        v >>= 1;
        pos++;
    }
    return pos;
}

int ffsl(long i) {
    unsigned long v = (unsigned long)i;
    int pos = 1;

    if (v == 0)
        return 0;
    while ((v & 1UL) == 0) {
        v >>= 1;
        pos++;
    }
    return pos;
}

int ffsll(long long i) {
    unsigned long long v = (unsigned long long)i;
    int pos = 1;

    if (v == 0)
        return 0;
    while ((v & 1ULL) == 0) {
        v >>= 1;
        pos++;
    }
    return pos;
}
