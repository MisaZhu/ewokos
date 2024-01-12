#include <fcntl.h>
#include <ewoksys/syscall.h>
#include <ewoksys/devcmd.h>
#include <ewoksys/vfs.h>
#include <stddef.h>
#include <string.h>

int open(const char* fname, int oflag) {
	int fd = -1;
	bool created = false;
	fsinfo_t info;

	if(vfs_get_by_name(fname, &info) != 0) {
		if((oflag & O_CREAT) != 0) {
			if(vfs_create(fname, &info, FS_TYPE_FILE, 0664, false, false) != 0)
				return -1;
			created = true;
		}
		else  {
			return -1;	
		}	
	}

	fd = vfs_open(&info, oflag);
	if(fd < 0) {
		if(created)
			vfs_del_node(info.node);
		return -1;
	}

	if(dev_open(info.mount_pid, fd, &info, oflag) != 0) {
		vfs_close(fd);
		if(created)
			vfs_del_node(info.node);
		fd = -1;
	}
	return fd;
}

