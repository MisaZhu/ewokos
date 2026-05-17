#include <stdlib.h>
#include <sys/errno.h>

char *tempnam(const char *dir, const char *pfx) {
	(void)dir;
	(void)pfx;
	errno = ENOSYS;
	return NULL;
}
