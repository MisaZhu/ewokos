#include <unistd.h>
#include <errno.h>
#include <ewoksys/vfs.h>

/* O_CLOEXEC is not tracked by the vfs layer, so the flags are accepted
 * but have no effect. */
int dup3(int oldfd, int newfd, int flags) {
    (void)flags;
    if (oldfd == newfd) {
        errno = EINVAL;
        return -1;
    }
    return vfs_dup2(oldfd, newfd);
}

int pipe2(int pipefd[2], int flags) {
    (void)flags;
    return vfs_open_pipe(pipefd);
}
