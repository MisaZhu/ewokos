#include <sys/stat.h>
#include <errno.h>
#include <ewoksys/vfs.h>

int mkdir(const char* name, mode_t mode) {
    int r = vfs_create(name, NULL, FS_TYPE_DIR, mode, false, true);
    if (r == 0)
        return 0;
    /* vfs_create reports failure without setting errno; derive a POSIX error
     * code so callers (and strerror) get a meaningful reason instead of 0. */
    struct stat st;
    if (stat(name, &st) == 0)
        errno = EEXIST;
    else
        errno = EIO;
    return -1;
}
