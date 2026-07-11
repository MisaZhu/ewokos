#include <sys/stat.h>

int lstat(const char *name, struct stat *buf) {
	return stat(name, buf);
}
