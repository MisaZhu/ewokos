#include <sys/vfs.h>
#include <fcntl.h>

int stat(const char* name, struct stat* buf) {
	fsinfo_t info;
	if(vfs_get(name, &info) != 0)
		return -1;
	
	buf->st_uid = info.node->uid;
	buf->st_gid = info.node->gid;
	buf->st_size = info.node->size;
	return 0;
}

