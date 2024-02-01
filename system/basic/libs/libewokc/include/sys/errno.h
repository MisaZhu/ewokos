#ifndef ERROR_NO_H
#define ERROR_NO_H

extern int errno;

#define VFS_ERR_RETRY -2

enum {
	ENONE = 0,
	EAGAIN,
	ENOTEMPTY,
	ENOENT,
	EPERM,
	EEXIST
};

#endif

