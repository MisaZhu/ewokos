#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/vfs.h>
#include <stddef.h>
#include <string.h>

int open(const char* fname, int oflag) {
	int fd = -1;
	bool created = false;
	fsinfo_t info;

	if(vfs_get_by_name(fname, &info) != 0) {
		if((oflag & O_CREAT) != 0) {
			if(vfs_create(fname, &info, FS_TYPE_FILE, false, false) != 0)
				return -1;
			created = true;
		}
		else  {
			return -1;	
		}	
	}

	fd = vfs_open(info.node, oflag);
	if(fd < 0) {
		if(created)
			vfs_del(&info);
		return -1;
	}
	
	proto_t in, out;
	PF->init(&out);

	PF->init(&in)->
		addi(&in, fd)->
		addi(&in, info.node)->
		addi(&in, oflag);

	if(ipc_call(info.mount_pid, FS_CMD_OPEN, &in, &out) != 0 ||
			proto_read_int(&out) != 0) {
		vfs_close(fd);
		if(created)
			vfs_del(&info);
		fd = -1;
	}

	PF->clear(&in);
	PF->clear(&out);
	return fd;
}

