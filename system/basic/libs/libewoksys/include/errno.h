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

