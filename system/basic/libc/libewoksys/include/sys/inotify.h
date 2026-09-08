#ifndef SYS_INOTIFY_H
#define SYS_INOTIFY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* EwokOS has no kernel inotify; these constants and the event layout exist so
 * that portable code compiles, and the syscalls below fail with ENOSYS so
 * callers can degrade gracefully. */
#define IN_ACCESS        0x00000001
#define IN_MODIFY        0x00000002
#define IN_ATTRIB        0x00000004
#define IN_CLOSE_WRITE   0x00000008
#define IN_CLOSE_NOWRITE 0x00000010
#define IN_OPEN          0x00000020
#define IN_MOVED_FROM    0x00000040
#define IN_MOVED_TO      0x00000080
#define IN_CREATE        0x00000100
#define IN_DELETE        0x00000200
#define IN_DELETE_SELF   0x00000400
#define IN_MOVE_SELF     0x00000800
#define IN_MOVE          (IN_MOVED_FROM | IN_MOVED_TO)
#define IN_CLOSE         (IN_CLOSE_WRITE | IN_CLOSE_NOWRITE)
#define IN_UNMOUNT       0x00002000
#define IN_Q_OVERFLOW    0x00004000
#define IN_IGNORED       0x00008000
#define IN_ISDIR         0x40000000

struct inotify_event {
	int      wd;
	uint32_t mask;
	uint32_t cookie;
	uint32_t len;
	char     name[];
};

int inotify_init(void);
int inotify_init1(int flags);
int inotify_add_watch(int fd, const char *path, uint32_t mask);
int inotify_rm_watch(int fd, int wd);

#ifdef __cplusplus
}
#endif

#endif
