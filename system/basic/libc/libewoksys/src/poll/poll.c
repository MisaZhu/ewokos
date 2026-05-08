#include <ewoksys/vfs.h>
#include <poll.h>

int poll(struct pollfd* fds, uint32_t nfds, int timeout) {
    if(fds == NULL || nfds == 0)
        return -1;
    int res = vfs_poll(fds, nfds, timeout);
    return res;
}