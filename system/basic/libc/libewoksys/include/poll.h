#ifndef POLL_H
#define POLL_H

#include <stdint.h>
#include <ewoksys/vfs.h>

typedef unsigned long nfds_t;
typedef vfs_pollfd_t pollfd_t;

#ifdef __cplusplus
extern "C" {
#endif

#undef POLLRW
#define POLLRW          VFS_EVT_RW

#undef POLLIN
#define POLLIN          VFS_EVT_RD

#undef POLLOUT
#define POLLOUT         VFS_EVT_WR

#undef POLLHUP
#define POLLHUP         VFS_EVT_CLOSE

#undef POLLERR
#define POLLERR         VFS_EVT_ERR

#undef POLLNVAL
#define POLLNVAL        VFS_EVT_NVAL

/*
 * POLLPRI is "exceptional condition" - out-of-band data on a socket, and on
 * Linux also the in-band signalling used by poll() to report that a device
 * node became readable without data (sysfs attributes, /proc files).
 *
 * EwokOS's VFS has no such event: VFS_EVT_* covers readable, writable,
 * read/write, closed, error and invalid-fd, and nothing more.  Defining it
 * to 0 rather than leaving it out is the honest encoding of that - a bit
 * that is never set means no fd will ever be reported with POLLPRI, and
 * poll() will simply not raise it.  It must still *exist*, because callers
 * build their event masks with it unconditionally:
 *
 *   qeventdispatcher_unix.cpp   revents & POLLPRI   (x3, in the socket
 *                                                   notifier dispatch)
 *   qcoreapplication.cpp        the same test on the self-pipe
 *
 * A missing macro there is a hard compile error, and inventing a nonzero
 * value that overlaps a real VFS_EVT_* bit would be worse: it would make
 * poll() report exceptional conditions that did not happen.
 */
#undef POLLPRI
#define POLLPRI         0

int poll(struct pollfd* fds, nfds_t nfds, int timeout);

#ifdef __cplusplus
}
#endif

#endif
