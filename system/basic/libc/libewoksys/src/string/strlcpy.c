#include <string.h>

size_t strlcpy(char* dst, const char* src, size_t size) {
    size_t src_len = strlen(src);
    if (size > 0) {
        size_t copy_len = src_len < size - 1 ? src_len : size - 1;
        memcpy(dst, src, copy_len);
        dst[copy_len] = '\0';
    }
    return src_len;
}
