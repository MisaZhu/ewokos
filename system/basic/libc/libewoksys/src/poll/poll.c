#include <ewoksys/vfs.h>
#include <poll.h>

int poll(struct pollfd* fds, uint32_t nfds, int timeout) {
    if(fds == NULL || nfds == 0)
        return -1;
    vfs_pollfd_t* vfs_fds = (vfs_pollfd_t*)malloc(nfds);
    for(uint32_t i = 0; i < nfds; i++) {
        vfs_fds[i].fd = fds[i].fd;
        vfs_fds[i].events = fds[i].events;
        vfs_fds[i].revents = 0;
        vfs_fds[i].node = 0;
    }
    return vfs_poll(vfs_fds, nfds, timeout);
}