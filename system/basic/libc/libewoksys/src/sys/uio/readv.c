#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    ssize_t total = 0;

    if (iov == NULL || iovcnt < 0) {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < iovcnt; i++) {
        ssize_t n;

        if (iov[i].iov_len == 0)
            continue;
        n = read(fd, iov[i].iov_base, iov[i].iov_len);
        if (n < 0)
            return (total > 0) ? total : -1;
        total += n;
        if ((size_t)n < iov[i].iov_len)
            break; /* short read: EOF or nonblocking exhaustion */
    }
    return total;
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    ssize_t total = 0;

    if (iov == NULL || iovcnt < 0) {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < iovcnt; i++) {
        ssize_t n;

        if (iov[i].iov_len == 0)
            continue;
        n = write(fd, iov[i].iov_base, iov[i].iov_len);
        if (n < 0)
            return (total > 0) ? total : -1;
        total += n;
        if ((size_t)n < iov[i].iov_len)
            break; /* short write */
    }
    return total;
}
