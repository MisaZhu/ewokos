#include <sys/stat.h>
#include <ewoksys/vfs.h>

int mkdir(const char* name, int mode) {
	return vfs_create(name, NULL, FS_TYPE_DIR, mode, false, true);
}

