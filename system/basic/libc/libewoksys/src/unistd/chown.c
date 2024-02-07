#include <fcntl.h>
#include <ewoksys/vfs.h>
#include <ewoksys/devcmd.h>
#include <stddef.h>
#include <string.h>

int chown(const char *pathname, int uid, int gid) {
	fsinfo_t info;
	if(vfs_get_by_name(pathname, &info) != 0)
		return -1;

	if(uid >= 0)
		info.stat.uid = uid;
	if(gid >= 0)
		info.stat.gid = gid;

	return vfs_update(&info);
}
