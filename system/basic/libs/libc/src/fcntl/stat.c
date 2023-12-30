#include <ewoksys/vfs.h>
#include <fcntl.h>

int stat(const char* name, struct stat* buf) {
	fsinfo_t info;
	if(vfs_get_by_name(name, &info) != 0)
		return -1;
	
	buf->st_uid = info.uid;
	buf->st_gid = info.gid;
	buf->st_size = info.size;
	return 0;
}

