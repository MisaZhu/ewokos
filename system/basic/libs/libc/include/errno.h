#ifndef ERROR_NO_H
#define ERROR_NO_H

extern int errno;

enum {
	ENONE = 0,
	EAGAIN,
	EAGAIN_NON_BLOCK,
	ENOTEMPTY
};

#endif

