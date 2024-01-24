#ifndef ERROR_NO_H
#define ERROR_NO_H

extern int errno;

#undef	ENONE
#undef	EAGAIN
#undef	EAGAIN_NON_BLOCK
#undef	ENOTEMPTY
#undef	ENOENT
#undef	EPERM
#undef	EEXIST

#define ERR_RETRY -2
#define ERR_RETRY_NON_BLOCK -3

enum {
	ENONE = 0,
	EAGAIN,
	EAGAIN_NON_BLOCK,
	ENOTEMPTY,
	ENOENT,
	EPERM,
	EEXIST
};

#endif

