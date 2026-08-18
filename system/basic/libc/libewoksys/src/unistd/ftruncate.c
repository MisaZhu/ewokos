#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>

/*
 * The vfs layer has no truncate protocol, so shrinking is not possible.
 * Growing is emulated by appending zero bytes.
 */
int ftruncate(int fd, off_t length) {
    char zero[256];
    off_t cur;

    if (fd < 0 || length < 0) {
        errno = EINVAL;
        return -1;
    }

    cur = lseek(fd, 0, SEEK_END);
    if (cur < 0) {
        errno = EBADF;
        return -1;
    }
    if (length == cur)
        return 0;
    if (length < cur) {
        errno = ENOSYS;
        return -1;
    }

    memset(zero, 0, sizeof(zero));
    while (cur < length) {
        size_t chunk = sizeof(zero);
        if ((off_t)chunk > length - cur)
            chunk = (size_t)(length - cur);
        if (write(fd, zero, chunk) != (ssize_t)chunk)
            return -1;
        cur += (off_t)chunk;
    }
    return 0;
}
