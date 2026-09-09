#include <fcntl.h>

int creat(const char *pathname, mode_t mode) {
	return open(pathname, O_WRONLY | O_CREAT | O_TRUNC, mode);
}
