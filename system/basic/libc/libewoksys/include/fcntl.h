#ifndef EWOKOS_LIBC_FCNTL_H
#define EWOKOS_LIBC_FCNTL_H

#include <stdarg.h>

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_NONBLOCK 0x0004
#define O_EXCL   0x0008
#define O_CREAT  0x0040
#define O_TRUNC  0x0200
#define O_APPEND 0x0400

#define F_GETFL  3
#define F_SETFL  4

#ifdef __cplusplus
extern "C" {
#endif

int open(const char *pathname, int flags, ...);
int fcntl(int fd, int cmd, ...);
int chmod(const char *pathname, int mode);
int fchmod(int fd, int mode);

#ifdef __cplusplus
}
#endif

#endif
