#include <unistd.h>
#include <sys/proc.h>
#include <sys/vfs.h>

int read(int fd, void* buf, uint32_t size) {
	errno = ENONE;
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

	int res = -1;
	if(info.type == FS_TYPE_PIPE) {
		while(1) {
			res = vfs_read_pipe(&info, buf, size, 1);
			if(res >= 0)
				break;
			if(errno != EAGAIN)
				break;
			sleep(0);
		}
		return res;
	}

	while(1) {
		res = vfs_read(fd, &info, buf, size);
		if(res >= 0)
			break;
		if(errno != EAGAIN)
			break;
		if((info.type & FS_TYPE_SYNC) != 0)
			proc_block(info.mount_pid, 0);
		else
			sleep(0);
	}
	return res;
}

