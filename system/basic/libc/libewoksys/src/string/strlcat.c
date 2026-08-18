#include <string.h>

size_t strlcat(char *dest, const char *src, size_t size) {
    size_t dest_len = 0;
    size_t src_len = strlen(src);
    size_t i;

    while (dest_len < size && dest[dest_len] != '\0') {
        dest_len++;
    }
    if (dest_len == size) {
        return size + src_len;
    }

    for (i = 0; src[i] != '\0' && dest_len + i + 1 < size; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';
    return dest_len + src_len;
}
