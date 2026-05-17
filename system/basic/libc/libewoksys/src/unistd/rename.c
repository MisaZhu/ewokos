#include <unistd.h>
#include <sys/errno.h>

int rename(const char *oldpath, const char *newpath) {
	(void)oldpath;
	(void)newpath;
	errno = ENOSYS;
	return -1;
}
