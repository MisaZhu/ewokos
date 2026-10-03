#include <ewoksys/vfs.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <time.h>

int poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    /*
     * POSIX: a poll() with no descriptors (nfds == 0) is a portable way to
     * sleep for `timeout` milliseconds and must return 0, not an error.
     *
     * Route through nanosleep() rather than usleep(): nanosleep() prefers
     * proc_nsleep_precise(), which lands the wake on the requested deadline
     * instead of the next scheduler tick (~976us at timer_freq=1024). A 1ms
     * poll() with no fds is a common pacing idiom and used to overshoot by
     * up to a full tick; userspace usleep() stays tick-quantized on purpose
     * because driver polling loops depend on that quantum as pacing.
     */
    if(nfds == 0) {
        if(timeout > 0) {
            struct timespec ts;
            ts.tv_sec = timeout / 1000;
            ts.tv_nsec = (long)(timeout % 1000) * 1000000L;
            nanosleep(&ts, NULL);
        }
        return 0;
    }
    if(fds == NULL) {
        errno = EINVAL;
        return -1;
    }
    int res = vfs_poll(fds, nfds, timeout);
    return res;
}
