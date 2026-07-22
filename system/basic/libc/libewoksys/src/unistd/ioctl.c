#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <stdarg.h>

int ioctl(int fd, int cmd, ...) {
	va_list args;
	void *argp;

	va_start(args, cmd);
	argp = va_arg(args, void *);
	va_end(args);

	if (cmd == FIONBIO) {
		int flags;
		int enabled;

		if (argp == NULL) {
			errno = EINVAL;
			return -1;
		}

		flags = fcntl(fd, F_GETFL);
		if (flags < 0) {
			return -1;
		}
		enabled = (*(int *)argp != 0);
		if (enabled) {
			flags |= O_NONBLOCK;
		}
		else {
			flags &= ~O_NONBLOCK;
		}
		return fcntl(fd, F_SETFL, flags);
	}

	if (cmd == FIOCLEX || cmd == FIONCLEX) {
		int flags = fcntl(fd, F_GETFD);
		if (flags < 0) {
			return -1;
		}
		if (cmd == FIOCLEX) {
			flags |= FD_CLOEXEC;
		}
		else {
			flags &= ~FD_CLOEXEC;
		}
		return fcntl(fd, F_SETFD, flags);
	}

	errno = ENOTTY;
	return -1;
}
