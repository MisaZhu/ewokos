#include <dirent.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>

/* Return a file descriptor for the directory. EwokOS opendir() does
 * not open an fd, so one is lazily opened on the first dirfd() call. */

int dirfd(DIR* dirp) {
    if(dirp == NULL) {
        errno = EINVAL;
        return -1;
    }
    if(dirp->fd < 0)
        dirp->fd = open(dirp->name, O_RDONLY);
    return dirp->fd;
}
