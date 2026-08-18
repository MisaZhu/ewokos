#include <unistd.h>
#include <stdbool.h>
#include <ewoksys/vfs.h>
#include <ewoksys/fsinfo.h>
#include <sys/errno.h>

int fsync(int fd) {
    fsinfo_t info;

    if (vfs_get_by_fd(fd, &info) != 0) {
        errno = EBADF;
        return -1;
    }
    if (vfs_flush(fd, true) != 0) {
        errno = EIO;
        return -1;
    }
    return 0;
}

int fdatasync(int fd) {
    return fsync(fd);
}

void sync(void) {
    for (int fd = 0; fd < MAX_OPEN_FILE_PER_PROC; fd++) {
        fsinfo_t info;
        if (vfs_get_by_fd(fd, &info) == 0)
            vfs_flush(fd, false);
    }
}
