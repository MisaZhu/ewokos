#include <ewoksys/vfs.h>
#include <fcntl.h>
#include <string.h>

int stat(const char* name, struct stat* buf) {
	memset(buf, 0, sizeof(struct stat));

	fsinfo_t info;
	if(vfs_get_by_name(name, &info) != 0)
		return -1;
	
	buf->st_uid = info.uid;
	buf->st_gid = info.gid;
	buf->st_size = info.size;
	buf->st_mode = 0664;
	return 0;
}

