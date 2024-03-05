#include <fcntl.h>
#include <ewoksys/vfs.h>
#include <ewoksys/devcmd.h>
#include <stddef.h>
#include <string.h>

int  chmod(const char *pathname, int mode) {
	fsinfo_t info;
	if(vfs_get_by_name(pathname, &info) != 0)
		return -1;

	info.stat.mode = mode;
	return vfs_update(&info, true);
}
