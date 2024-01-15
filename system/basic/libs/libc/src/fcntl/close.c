#include <fcntl.h>
#include <ewoksys/syscall.h>
#include <ewoksys/devcmd.h>
#include <ewoksys/vfs.h>
#include <stddef.h>
#include <string.h>

void close(int fd) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return;

	dev_close(info.mount_pid, fd, info.node);
	vfs_close(fd);
}

