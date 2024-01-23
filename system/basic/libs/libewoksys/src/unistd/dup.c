#include <unistd.h>
#include <ewoksys/vfs.h>

int dup(int from) {
	return vfs_dup(from);
}

