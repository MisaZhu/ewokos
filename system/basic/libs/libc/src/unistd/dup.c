#include <unistd.h>
#include <sys/vfs.h>

int dup(int from) {
	return vfs_dup(from);
}

