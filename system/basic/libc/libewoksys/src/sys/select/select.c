#include <sys/select.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

static int timeval_to_poll_timeout(const struct timeval *timeout) {
	unsigned long long msec;

	if (timeout == NULL) {
		return -1;
	}
	if (timeout->tv_sec < 0 || timeout->tv_usec < 0 || timeout->tv_usec >= 1000000) {
		errno = EINVAL;
		return -2;
	}

	msec = (unsigned long long)timeout->tv_sec * 1000ULL +
		(unsigned long long)(timeout->tv_usec / 1000);
	if ((timeout->tv_usec % 1000) != 0) {
		msec++;
	}
	if (msec > 0x7fffffffULL) {
		msec = 0x7fffffffULL;
	}
	return (int)msec;
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		struct timeval *timeout) {
	struct pollfd *fds;
	int timeout_ms;
	int count = 0;
	int ready;

	if (nfds < 0 || nfds > FD_SETSIZE) {
		errno = EINVAL;
		return -1;
	}

	timeout_ms = timeval_to_poll_timeout(timeout);
	if (timeout_ms == -2) {
		return -1;
	}
	if (nfds == 0) {
		if (timeout_ms > 0) {
			usleep((useconds_t)timeout_ms * 1000U);
		}
		if (readfds != NULL) {
			FD_ZERO(readfds);
		}
		if (writefds != NULL) {
			FD_ZERO(writefds);
		}
		if (exceptfds != NULL) {
			FD_ZERO(exceptfds);
		}
		return 0;
	}

	fds = (struct pollfd *)malloc((size_t)nfds * sizeof(struct pollfd));
	if (fds == NULL) {
		errno = ENOMEM;
		return -1;
	}

	for (int fd = 0; fd < nfds; ++fd) {
		short events = 0;

		if (readfds != NULL && FD_ISSET(fd, readfds)) {
			events |= POLLIN;
		}
		if (writefds != NULL && FD_ISSET(fd, writefds)) {
			events |= POLLOUT;
		}
		if (exceptfds != NULL && FD_ISSET(fd, exceptfds)) {
			events |= POLLERR | POLLHUP | POLLNVAL;
		}

		fds[fd].fd = fd;
		fds[fd].events = (uint16_t)events;
		fds[fd].revents = 0;
	}

	ready = poll(fds, (nfds_t)nfds, timeout_ms);
	if (ready < 0) {
		free(fds);
		return -1;
	}

	if (readfds != NULL) {
		FD_ZERO(readfds);
	}
	if (writefds != NULL) {
		FD_ZERO(writefds);
	}
	if (exceptfds != NULL) {
		FD_ZERO(exceptfds);
	}

	for (int fd = 0; fd < nfds; ++fd) {
		int fd_ready = 0;

		if (readfds != NULL && (fds[fd].revents & (POLLIN | POLLHUP)) != 0) {
			FD_SET(fd, readfds);
			fd_ready = 1;
		}
		if (writefds != NULL && (fds[fd].revents & POLLOUT) != 0) {
			FD_SET(fd, writefds);
			fd_ready = 1;
		}
		if (exceptfds != NULL && (fds[fd].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
			FD_SET(fd, exceptfds);
			fd_ready = 1;
		}
		count += fd_ready;
	}

	free(fds);
	return count;
}
