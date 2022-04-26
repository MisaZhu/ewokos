#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/proc.h>
#include <sys/ipc.h>
#include <sys/proc.h>
#include <sys/vfsc.h>
#include <sys/vfs.h>
#include <stddef.h>
#include <string.h>

int fcntl(int fd, int cmd, int data) {
	int res = -1;
	if(cmd == F_GETFL) {
		ipc_call_simple(get_vfsd_pid(), VFS_GET_FLAGS, fd, 0, 0, 0, &res);
	}
	else if(cmd == F_SETFL) {
		ipc_call_simple(get_vfsd_pid(), VFS_SET_FLAGS, fd, data, 0, 0, &res);
	}
	return res;
}

