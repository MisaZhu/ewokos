#ifndef EWOKOS_SYS_STATVFS_H
#define EWOKOS_SYS_STATVFS_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ST_RDONLY 0x1
#define ST_NOSUID 0x2

struct statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	unsigned long f_blocks;
	unsigned long f_bfree;
	unsigned long f_bavail;
	unsigned long f_files;
	unsigned long f_ffree;
	unsigned long f_favail;
	unsigned long f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
};

int statvfs(const char *path, struct statvfs *buf);
int fstatvfs(int fd, struct statvfs *buf);

/*
 * The LFS names.  glibc provides statvfs64/fstatvfs64 as real, distinct
 * symbols for programs compiled with _FILE_OFFSET_BITS=64; EwokOS has one
 * off_t and one struct, so the 64-bit names are aliases for the same things.
 *
 * A macro is the right tool here rather than typedefs and wrappers: it makes
 * both `struct statvfs64 buf;` and a call to `::statvfs64(...)` resolve, with
 * no second symbol to link and no layout to keep in sync.
 *
 * qstorageinfo_unix.cpp:retrieveVolumeInfo() declares `statvfs64 statfs_buf`
 * and calls ::statvfs64 on the Linux path, which produced two errors:
 * `aggregate ... has incomplete type` and `'::statvfs64' has not been
 * declared`.
 */
#define statvfs64  statvfs
#define fstatvfs64 fstatvfs

#ifdef __cplusplus
}
#endif

#endif
