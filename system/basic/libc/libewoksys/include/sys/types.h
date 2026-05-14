#ifndef EWOKOS_LIBC_SYS_TYPES_H
#define EWOKOS_LIBC_SYS_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef unsigned int mode_t;
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
typedef long off_t;
typedef long _off_t;
typedef long ssize_t;
typedef long time_t;
typedef long clock_t;

typedef unsigned long dev_t;
typedef unsigned long ino_t;
typedef unsigned long nlink_t;
typedef long blksize_t;
typedef long blkcnt_t;

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;

#endif
