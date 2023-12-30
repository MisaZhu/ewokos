#include <unistd.h>
#include <fcntl.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfs.h>
#include <fcntl.h>

static int read_nblock(int fd, void* buf, uint32_t size) {
  errno = ENONE;
  fsinfo_t info;
  if(vfs_get_by_fd(fd, &info) != 0)
    return -1;
  if(info.type == FS_TYPE_PIPE)
    return vfs_read_pipe(info.node, buf, size, 0);
  return vfs_read(fd, &info, buf, size);
}

static int read_block(int fd, void* buf, uint32_t size) {
	errno = ENONE;
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return -1;

	int res = -1;
	if(info.type == FS_TYPE_PIPE) {
		while(1) {
			res = vfs_read_pipe(info.node, buf, size, true);
			if(res >= 0 || errno != EAGAIN)
				break;
		}
		return res;
	}

	while(1) {
		res = vfs_read(fd, &info, buf, size);
		if(res >= 0 || errno == EAGAIN_NON_BLOCK) {
			errno = EAGAIN;
			break;
		}

		if(errno == EAGAIN)
			proc_block(info.mount_pid, RW_BLOCK_EVT);
	}
	return res;
}

int read(int fd, void* buf, uint32_t size) {
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags == -1)
		return -1;

  if((flags & O_NONBLOCK) == 0)
		return read_block(fd, buf, size);
	return read_nblock(fd, buf, size);
}
