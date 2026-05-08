#ifndef POLL_H
#define POLL_H

#include <stdint.h>
#include <ewoksys/vfs.h>

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

#endif
