#include <ewoksys/vfs.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    /*
     * POSIX: a poll() with no descriptors (nfds == 0) is a portable way to
     * sleep for `timeout` milliseconds and must return 0, not an error.
     */
    if(nfds == 0) {
        if(timeout > 0)
            usleep((uint32_t)timeout * 1000);
        return 0;
    }
    if(fds == NULL) {
        errno = EINVAL;
        return -1;
    }
    int res = vfs_poll(fds, nfds, timeout);
    return res;
}
