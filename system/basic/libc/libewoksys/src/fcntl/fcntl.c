#include <fcntl.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include <ewoksys/vfsc.h>
#include <ewoksys/vfs.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

//int fcntl(int fd, int cmd, int data) {
int fcntl(int fd, int cmd, ...) {
	/*
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
	*/

	if(cmd == F_GETFL) {
		return vfs_get_flags(fd);
	}
	else if(cmd == F_SETFL) {
    va_list args;
    va_start(args, 0);
    int flags = va_arg(args, int);
    va_end(args);
		return vfs_set_flags(fd, flags);
	}
	else {
		proto_t in, out;
		PF->init(&in);
		PF->init(&out);

		va_list args;
    va_start(args, 0);
    int num = va_arg(args, int);
		for(int i=0; i<num; i++) 
			PF->addi(&in, va_arg(args, int));
    va_end(args);

		int res = vfs_fcntl(fd, cmd, &in, &out);
		PF->clear(&in);
		if(res != 0) {
			PF->clear(&out);
			return -1;
		}
		res = proto_read_int(&out);
		PF->clear(&out);
		return res;
	}
	return 0;
}

