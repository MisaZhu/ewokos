#include <sys/select.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
/* fsinfo_t / vfs_get_by_fd() come from <ewoksys/vfs.h>, pulled in by <poll.h>. */

/*
 * Convert a struct timeval timeout into the millisecond value poll() expects.
 * Returns:
 *   -1  => timeout == NULL, block indefinitely
 *   -2  => invalid timeout value, errno is set to EINVAL
 *   >=0 => timeout in milliseconds (capped to INT_MAX)
 */
static int timeval_to_poll_timeout(const struct timeval *timeout) {
    unsigned long long msec;

    if (timeout == NULL) {
        return -1;
    }
    if (timeout->tv_sec < 0 || timeout->tv_usec < 0 || timeout->tv_usec >= 1000000) {
        errno = EINVAL;
        return -2;
    }

    msec = (unsigned long long)timeout->tv_sec * 1000ULL +
        (unsigned long long)(timeout->tv_usec / 1000);
    if ((timeout->tv_usec % 1000) != 0) {
        msec++;
    }
    if (msec > 0x7fffffffULL) {
        msec = 0x7fffffffULL;
    }
    return (int)msec;
}

/* Same conversion as above, but for the struct timespec used by pselect(). */
static int timespec_to_poll_timeout(const struct timespec *timeout) {
    unsigned long long msec;

    if (timeout == NULL) {
        return -1;
    }
    if (timeout->tv_sec < 0 || timeout->tv_nsec < 0 || timeout->tv_nsec >= 1000000000L) {
        errno = EINVAL;
        return -2;
    }

    msec = (unsigned long long)timeout->tv_sec * 1000ULL +
        (unsigned long long)(timeout->tv_nsec / 1000000L);
    if ((timeout->tv_nsec % 1000000L) != 0) {
        msec++;
    }
    if (msec > 0x7fffffffULL) {
        msec = 0x7fffffffULL;
    }
    return (int)msec;
}

/*
 * POSIX requires select()/pselect() to fail with EBADF when any descriptor
 * named in the sets is not a valid open file descriptor. EwokOS' vfs_poll()
 * silently ignores invalid fds (it never reports POLLNVAL), so validate them
 * up front using the same criterion vfs_poll() uses internally:
 * vfs_get_by_fd() == 0 and a non-zero node.
 */
static int check_fds_valid(int nfds, const fd_set *readfds,
        const fd_set *writefds, const fd_set *exceptfds) {
    for (int fd = 0; fd < nfds; ++fd) {
        int in_set = 0;

        if (readfds != NULL && FD_ISSET(fd, readfds)) {
            in_set = 1;
        }
        if (writefds != NULL && FD_ISSET(fd, writefds)) {
            in_set = 1;
        }
        if (exceptfds != NULL && FD_ISSET(fd, exceptfds)) {
            in_set = 1;
        }
        if (!in_set) {
            continue;
        }

        fsinfo_t info;
        if (vfs_get_by_fd(fd, &info) != 0 || info.node == 0) {
            errno = EBADF;
            return -1;
        }
    }
    return 0;
}

/*
 * Shared core of select()/pselect(). `timeout_ms` is already normalised by one
 * of the *_to_poll_timeout() helpers (-1 blocks forever, >=0 is a millisecond
 * deadline). This is the single place that validates nfds, so the callers stay
 * thin and the malloc() below has a provably bounded size.
 */
static int do_select(int nfds, fd_set *readfds, fd_set *writefds,
        fd_set *exceptfds, int timeout_ms) {
    struct pollfd *fds;
    int count = 0;
    int ready;

    if (nfds < 0 || nfds > FD_SETSIZE) {
        errno = EINVAL;
        return -1;
    }

    if (nfds == 0) {
        /*
         * POSIX: with no descriptors this is a portable way to sleep for the
         * timeout; the sets (if any) are zeroed and 0 is returned.
         */
        if (timeout_ms > 0) {
            usleep((useconds_t)timeout_ms * 1000U);
        }
        if (readfds != NULL) {
            FD_ZERO(readfds);
        }
        if (writefds != NULL) {
            FD_ZERO(writefds);
        }
        if (exceptfds != NULL) {
            FD_ZERO(exceptfds);
        }
        return 0;
    }

    if (check_fds_valid(nfds, readfds, writefds, exceptfds) != 0) {
        return -1; /* errno = EBADF */
    }

    fds = (struct pollfd *)malloc((size_t)nfds * sizeof(struct pollfd));
    if (fds == NULL) {
        errno = ENOMEM;
        return -1;
    }

    for (int fd = 0; fd < nfds; ++fd) {
        short events = 0;

        if (readfds != NULL && FD_ISSET(fd, readfds)) {
            events |= POLLIN;
        }
        if (writefds != NULL && FD_ISSET(fd, writefds)) {
            events |= POLLOUT;
        }
        if (exceptfds != NULL && FD_ISSET(fd, exceptfds)) {
            events |= POLLERR | POLLHUP | POLLNVAL;
        }

        fds[fd].fd = fd;
        fds[fd].events = (uint16_t)events;
        fds[fd].revents = 0;
    }

    ready = poll(fds, (nfds_t)nfds, timeout_ms);
    if (ready < 0) {
        free(fds);
        return -1;
    }

    if (readfds != NULL) {
        FD_ZERO(readfds);
    }
    if (writefds != NULL) {
        FD_ZERO(writefds);
    }
    if (exceptfds != NULL) {
        FD_ZERO(exceptfds);
    }

    for (int fd = 0; fd < nfds; ++fd) {
        /*
         * POSIX: select() returns the total number of bits set across all
         * three descriptor objects, so an fd ready for several conditions is
         * counted once per set it ends up in (not once per fd).
         */
        if (readfds != NULL && (fds[fd].revents & (POLLIN | POLLHUP)) != 0) {
            FD_SET(fd, readfds);
            count++;
        }
        if (writefds != NULL && (fds[fd].revents & POLLOUT) != 0) {
            FD_SET(fd, writefds);
            count++;
        }
        if (exceptfds != NULL && (fds[fd].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            FD_SET(fd, exceptfds);
            count++;
        }
    }

    free(fds);
    return count;
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
        struct timeval *timeout) {
    int timeout_ms;

    timeout_ms = timeval_to_poll_timeout(timeout);
    if (timeout_ms == -2) {
        return -1;
    }

    return do_select(nfds, readfds, writefds, exceptfds, timeout_ms);
}

int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
        const struct timespec *timeout, const sigset_t *sigmask) {
    int timeout_ms;
    sigset_t oldset;
    int have_oldset = 0;
    int ret;
    int saved_errno;

    timeout_ms = timespec_to_poll_timeout(timeout);
    if (timeout_ms == -2) {
        return -1;
    }

    /*
     * POSIX: pselect() atomically swaps in sigmask for the duration of the
     * wait and restores the previous mask on return. EwokOS only delivers
     * STOP/KILL from the kernel and keeps the rest of the mask as user-space
     * bookkeeping, but honouring the swap keeps the semantics (and ported
     * code relying on it) correct.
     */
    if (sigmask != NULL) {
        if (sigprocmask(SIG_SETMASK, sigmask, &oldset) == 0) {
            have_oldset = 1;
        }
    }

    ret = do_select(nfds, readfds, writefds, exceptfds, timeout_ms);
    saved_errno = errno;

    if (have_oldset) {
        sigprocmask(SIG_SETMASK, &oldset, NULL);
    }

    errno = saved_errno;
    return ret;
}
