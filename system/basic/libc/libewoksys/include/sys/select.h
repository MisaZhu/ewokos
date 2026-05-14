#ifndef EWOKOS_LIBC_SYS_SELECT_H
#define EWOKOS_LIBC_SYS_SELECT_H

#include <sys/types.h>
#include <sys/time.h>

#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

#ifndef NFDBITS
#define NFDBITS ((int)(8 * sizeof(unsigned long)))
#endif

typedef struct fd_set {
	unsigned long fds_bits[(FD_SETSIZE + NFDBITS - 1) / NFDBITS];
} fd_set;

#define FD_ZERO(set) \
	do { \
		for (size_t __i = 0; __i < sizeof((set)->fds_bits) / sizeof((set)->fds_bits[0]); ++__i) { \
			(set)->fds_bits[__i] = 0; \
		} \
	} while (0)

#define FD_SET(fd, set) \
	((set)->fds_bits[(fd) / NFDBITS] |= (1UL << ((fd) % NFDBITS)))

#define FD_CLR(fd, set) \
	((set)->fds_bits[(fd) / NFDBITS] &= ~(1UL << ((fd) % NFDBITS)))

#define FD_ISSET(fd, set) \
	(((set)->fds_bits[(fd) / NFDBITS] & (1UL << ((fd) % NFDBITS))) != 0)

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		struct timeval *timeout);

#endif
