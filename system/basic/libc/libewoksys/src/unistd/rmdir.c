#include <unistd.h>
#include <sys/errno.h>

int rmdir(const char *pathname) {
	(void)pathname;
	errno = ENOSYS;
	return -1;
}
