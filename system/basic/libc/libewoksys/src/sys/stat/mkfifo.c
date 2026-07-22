#include <sys/stat.h>
#include <ewoksys/vfs.h>

int mkfifo(const char* name, mode_t mode) {
	return vfs_create(name, NULL, FS_TYPE_PIPE, mode, true, true);
}
