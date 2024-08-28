#include <sys/stat.h>
#include <ewoksys/vfs.h>

int mkfifo(const char* name, int mode) {
	return vfs_create(name, NULL, FS_TYPE_PIPE, mode, true, true);
}

