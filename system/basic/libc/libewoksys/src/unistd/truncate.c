#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>

int truncate(const char *path, off_t length) {
    int fd;
    int ret;

    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    fd = open(path, O_WRONLY);
    if (fd < 0)
        return -1;
    ret = ftruncate(fd, length);
    close(fd);
    return ret;
}
