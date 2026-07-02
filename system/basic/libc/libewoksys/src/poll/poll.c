#include <ewoksys/vfs.h>
#include <unistd.h>
#include <poll.h>

int poll(struct pollfd* fds, uint32_t nfds, int timeout) {
    /*
     * POSIX: a poll() with no descriptors (nfds == 0) is a portable way to
     * sleep for `timeout` milliseconds and must return 0, not an error.
     */
    if(nfds == 0) {
        if(timeout > 0)
            usleep((uint32_t)timeout * 1000);
        return 0;
    }
    if(fds == NULL)
        return -1;
    int res = vfs_poll(fds, nfds, timeout);
    return res;
}
