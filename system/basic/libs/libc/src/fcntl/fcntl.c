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
	proto_t in, out;
	PF->init(&in, NULL, 0);
	PF->init(&out, NULL, 0);

	if(cmd == F_GETFL) {
		PF->addi(&in, fd);
		if(ipc_call(get_vfsd_pid(), VFS_GET_FLAGS, &in, &out) == 0) {
			if(proto_read_int(&out) == 0)
				res = proto_read_int(&out);
		}
	}
	else if(cmd == F_SETFL) {
		PF->addi(&in, fd)->addi(&in, data);
		if(ipc_call(get_vfsd_pid(), VFS_SET_FLAGS, &in, &out) == 0) {
			if(proto_read_int(&out) == 0)
				res = 0;
		}
	}

	PF->clear(&out);
	PF->clear(&in);
	return res;
}

