#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int closedir(DIR* dirp) {
    if(dirp == NULL) {
        errno = EBADF;
        return -1;
    }
    if(dirp->fd >= 0)
        close(dirp->fd);
    if(dirp->kids != NULL)
        free(dirp->kids);
    free(dirp);
    return 0;
}
