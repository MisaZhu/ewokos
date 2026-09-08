#ifndef ERROR_NO_H
#define ERROR_NO_H

#ifdef __cplusplus
extern "C" {
#endif

int *__errno(void);

#ifdef __cplusplus
}
#endif

#ifndef errno
#define errno (*__errno())
#endif

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
#undef  EAFNOSUPPORT

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
	EBUSY,
	EIO,
	EBADF,
	EINTR,
	EPIPE,
	E2BIG,
	EACCES,
	ECHILD,
	EDEADLK,
	EFAULT,
	EMFILE,
	EMLINK,
	ENAMETOOLONG,
	ENFILE,
	ENODEV,
	ENOEXEC,
	ENOLCK,
	ENOSYS,
	ENOSPC,
	ENOTTY,
	EISDIR,
	ENOTDIR,
	ENXIO,
	ESRCH,
	EILSEQ,
	ENOTSUP,
	EFBIG,
	EROFS,
	ESPIPE,
	EXDEV,
	EDOM
};

#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT 97
#endif

/* common aliases and socket-related codes (Linux-compatible values) */
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif
#ifndef EDEADLOCK
#define EDEADLOCK EDEADLK
#endif
#ifndef ENOMSG
#define ENOMSG 42
#endif
#ifndef EMSGSIZE
#define EMSGSIZE 90
#endif
#ifndef EPROTOTYPE
#define EPROTOTYPE 91
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT 92
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT 93
#endif
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT 94
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 95
#endif
#ifndef ENOTSUP
#define ENOTSUP EOPNOTSUPP
#endif
#ifndef EPFNOSUPPORT
#define EPFNOSUPPORT 96
#endif
#ifndef EADDRINUSE
#define EADDRINUSE 98
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL 99
#endif
#ifndef ENETDOWN
#define ENETDOWN 100
#endif
#ifndef ENETUNREACH
#define ENETUNREACH 101
#endif
#ifndef ENETRESET
#define ENETRESET 102
#endif
#ifndef ECONNABORTED
#define ECONNABORTED 103
#endif
#ifndef ECONNRESET
#define ECONNRESET 104
#endif
#ifndef ENOBUFS
#define ENOBUFS 105
#endif
#ifndef EISCONN
#define EISCONN 106
#endif
#ifndef ENOTCONN
#define ENOTCONN 107
#endif
#ifndef ESHUTDOWN
#define ESHUTDOWN 108
#endif
#ifndef ETOOMANYREFS
#define ETOOMANYREFS 109
#endif
#ifndef EHOSTDOWN
#define EHOSTDOWN 112
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH 113
#endif
#ifndef EALREADY
#define EALREADY 114
#endif
#ifndef EINPROGRESS
#define EINPROGRESS 115
#endif
#ifndef ESTALE
#define ESTALE 116
#endif
#ifndef ENODATA
#define ENODATA 61
#endif
#ifndef EOVERFLOW
#define EOVERFLOW 75
#endif
#ifndef ECANCELED
#define ECANCELED 125
#endif

#endif
