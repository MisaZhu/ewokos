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
#undef	EINVAL
#undef	ENOMEM
#undef	EBUSY

enum {
	ENONE = 0,
	EAGAIN,
	ENOTEMPTY,
	ENOENT,
	EPERM,
	ERANGE,
	ETIMEDOUT,
	EEXIST,
	EINVAL,
	ENOMEM,
	EBUSY
};

#endif

