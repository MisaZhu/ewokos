#ifndef EWOKOS_LIBC_SYS_REENT_H
#define EWOKOS_LIBC_SYS_REENT_H

#ifdef __cplusplus
extern "C" {
#endif

#define __IMPORT

struct _reent {
	int _errno;
	void (*__cleanup)(struct _reent *);
};

#define _REENT_ERRNO(ptr) ((ptr)->_errno)

#ifdef __cplusplus
}
#endif

#endif
