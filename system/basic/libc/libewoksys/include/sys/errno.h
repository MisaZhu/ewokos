#ifndef ERROR_NO_H
#define ERROR_NO_H

extern int errno;

#undef	ENONE
#undef	EAGAIN
#undef	ENOTEMPTY
#undef	ENOENT
#undef	EPERM
#undef	EEXIST
#undef	ERANGE
#undef	ETIMEDOUT

enum {
	ENONE = 0,
	EAGAIN,
	ENOTEMPTY,
	ENOENT,
	EPERM,
	ERANGE,
	ETIMEDOUT,
	EEXIST
};

#endif

