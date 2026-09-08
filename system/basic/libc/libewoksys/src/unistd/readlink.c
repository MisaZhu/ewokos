#include <unistd.h>
#include <errno.h>

/* EwokOS VFS has no symbolic links; readlink always fails with ENOSYS. */
ssize_t readlink(const char *path, char *buf, size_t bufsiz)
{
	(void)path; (void)buf; (void)bufsiz;
	errno = ENOSYS;
	return -1;
}
