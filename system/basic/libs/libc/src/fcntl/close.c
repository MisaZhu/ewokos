#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/ipc.h>
#include <sys/vfs.h>
#include <stddef.h>
#include <string.h>

void close(int fd) {
	fsinfo_t info;
	if(vfs_get_by_fd(fd, &info) != 0)
		return;

	proto_t in;
	PF->init(&in)->
		addi(&in, fd)->
		addi(&in, -1)->
		add(&in, &info, sizeof(fsinfo_t));

	ipc_call(info.mount_pid, FS_CMD_CLOSE, &in, NULL);
	PF->clear(&in);
	vfs_close(fd);
}

