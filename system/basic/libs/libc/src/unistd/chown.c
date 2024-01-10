#include <fcntl.h>
#include <ewoksys/vfs.h>
#include <ewoksys/devcmd.h>
#include <stddef.h>
#include <string.h>

int chown(const char *pathname, int uid, int gid) {
	fsinfo_t info;
	if(vfs_get_by_name(pathname, &info) != 0)
		return -1;
	info.stat.uid = uid;
	info.stat.gid = gid;
	return dev_set(info.mount_pid, &info);
}
