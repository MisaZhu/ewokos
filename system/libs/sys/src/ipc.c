#include <sys/ipc.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <rawdata.h>
#include <unistd.h>

int ipc_setup(ipc_handle_t handle, void* p, bool prefork) {
	return syscall3(SYS_IPC_SETUP, (int32_t)handle, (int32_t)p, (int32_t)prefork);
}

int ipc_set_return(const proto_t* pkg) {
	syscall1(SYS_IPC_SET_RETURN, (int32_t)pkg);
	return 0;
}

void ipc_end(void) {
	syscall0(SYS_IPC_END);
}

proto_t* ipc_get_arg(void) {
	return (proto_t*)syscall0(SYS_IPC_GET_ARG);
}

int ipc_call(int to_pid, int call_id, const proto_t* ipkg, proto_t* opkg) {
	if(to_pid < 0)
		return -1;

	int res = 0;
	while(true) {
		int res = syscall3(SYS_IPC_CALL, (int32_t)to_pid, (int32_t)call_id, (int32_t)ipkg);
		if(res == 0)
			break;
		if(res == -2)
			return -1;
		sleep(0); //retry	
	}

	proto_clear(opkg);
	while(true) {
		res = syscall2(SYS_IPC_GET_RETURN, (int32_t)to_pid, (int32_t)opkg);
		if(res == 0)
			break;
		if(res == -2)
			return -1;
		sleep(0); //retry	
	}
	return 0;
}

