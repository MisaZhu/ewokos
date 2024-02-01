#include <unistd.h>
#include <fcntl.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>

static int write_nblock(int fd, const void* buf, uint32_t size) {
  errno = ENONE;
  fsinfo_t info;
  if(vfs_get_by_fd(fd, &info) != 0)
    return -1;
  if(info.type == FS_TYPE_PIPE)
    return vfs_write_pipe(fd, info.node, buf, size, 0);
  return vfs_write(fd, &info, buf, size);
}

static int write_block(int fd, const void* buf, uint32_t size) {
	errno = ENONE;
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

	int res = -1;
	if(info.type == FS_TYPE_PIPE) {
		while(1) {
			res = vfs_write_pipe(fd, info.node, buf, size, true);
			if(res >= 0 || errno != EAGAIN)
				break;
		}
		return res;
	}

	while(1) {
		res = vfs_write(fd, &info, buf, size);
		if(res >= 0) 
			break;

		if(errno != EAGAIN)
			break;
		proc_block_by(info.mount_pid, RW_BLOCK_EVT);
	}
	return res;
}

int write(int fd, const void* buf, uint32_t size) {
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1)
		return -1;

  if((flags & O_NONBLOCK) == 0)
		return write_block(fd, buf, size);
	return write_nblock(fd, buf, size);
}
