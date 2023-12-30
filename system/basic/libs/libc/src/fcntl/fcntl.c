#include <fcntl.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/vfs.h>
#include <stddef.h>
#include <string.h>

int fcntl(int fd, int cmd, int data) {
	int res = -1;
	proto_t in, out;
	PF->init(&out);

	if(cmd == F_GETFL) {
		PF->init(&in)->addi(&in, fd);
		if(ipc_call(get_vfsd_pid(), VFS_GET_FLAGS, &in, &out) == 0) {
			if(proto_read_int(&out) == 0)
				res = proto_read_int(&out);
		}
	}
	else if(cmd == F_SETFL) {
		PF->init(&in)->addi(&in, fd)->addi(&in, data);
		if(ipc_call(get_vfsd_pid(), VFS_SET_FLAGS, &in, &out) == 0) {
			if(proto_read_int(&out) == 0)
				res = 0;
		}
	}

	PF->clear(&out);
	PF->clear(&in);
	return res;
}

