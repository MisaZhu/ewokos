#include <sys/statvfs.h>
#include <stddef.h>
#include <errno.h>

/* The vfs layer exposes no filesystem statistics; report fixed guesses. */
static void statvfs_fill(struct statvfs *buf) {
    buf->f_bsize = 4096;
    buf->f_frsize = 4096;
    buf->f_blocks = 0;
    buf->f_bfree = 0;
    buf->f_bavail = 0;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_favail = 0;
    buf->f_fsid = 0;
    buf->f_flag = 0;
    buf->f_namemax = 255;
}

int statvfs(const char *path, struct statvfs *buf) {
    if (path == NULL || buf == NULL) {
        errno = EINVAL;
        return -1;
    }
    statvfs_fill(buf);
    return 0;
}

int fstatvfs(int fd, struct statvfs *buf) {
    if (fd < 0 || buf == NULL) {
        errno = EBADF;
        return -1;
    }
    statvfs_fill(buf);
    return 0;
}
