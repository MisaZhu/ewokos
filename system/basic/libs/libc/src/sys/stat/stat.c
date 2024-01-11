#include <ewoksys/vfs.h>
#include <sys/stat.h>
#include <string.h>

int stat(const char* name, struct stat* buf) {
	memset(buf, 0, sizeof(struct stat));

	fsinfo_t info;
	if(vfs_get_by_name(name, &info) != 0) {
		return -1;
	}
	
	buf->st_uid = info.stat.uid;
	buf->st_gid = info.stat.gid;
	buf->st_size = info.stat.size;
	buf->st_mode = info.stat.mode;
	buf->st_atime = info.stat.atime;
	buf->st_ctime = info.stat.ctime;
	buf->st_mtime = info.stat.mtime;
	buf->st_nlink = info.stat.links_count;
	return 0;
}

