#include <sys/ipc.h>
#include <sys/syscall.h>
#include <sys/core.h>
#include <sys/proc.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


int ipc_setup(ipc_handle_t handle, void* p, bool nonblock) {
	return syscall3(SYS_IPC_SETUP, (int32_t)handle, (int32_t)p, (int32_t)nonblock);
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
		if(res == -2) {
			return -1;
		}
		sleep(0); //retry	
	}

	if(opkg != NULL)
		PF->clear(opkg);

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

/*----ipc server ------*/

int ipc_serv_reg(const char* ipc_serv_id) {
	int core_pid = syscall0(SYS_CORE_PID);
	if(core_pid < 0)
		return -1;

	int res = -1;
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->adds(&in, ipc_serv_id);
	if(ipc_call(core_pid, CORE_CMD_IPC_SERV_REG, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int ipc_serv_get(const char* ipc_serv_id) {
	int core_pid = syscall0(SYS_CORE_PID);
	if(core_pid < 0)
		return -1;

	int res = -1;
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->adds(&in, ipc_serv_id);
	if(ipc_call(core_pid, CORE_CMD_IPC_SERV_GET, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int ipc_serv_unreg(const char* ipc_serv_id) {
	int core_pid = syscall0(SYS_CORE_PID);
	if(core_pid < 0)
		return -1;

	int res = -1;
	proto_t in, out;
	PF->init(&out, NULL, 0);

	PF->init(&in, NULL, 0)->adds(&in, ipc_serv_id);
	if(ipc_call(core_pid, CORE_CMD_IPC_SERV_UNREG, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

static ipc_serv_handle_t _ipc_serv_handle;

static void handle_ipc(int pid, int cmd, void* p) {
	proto_t* in = ipc_get_arg();

	proto_t out;
	PF->init(&out, NULL, 0);

	_ipc_serv_handle(pid, cmd, in, &out, p);

	proto_free(in);
	ipc_set_return(&out);
	PF->clear(&out);
	ipc_end();
}

int ipc_serv_run(ipc_serv_handle_t handle, void* p, bool nonblock) {
	_ipc_serv_handle = handle;

	proc_ready_ping();
	return ipc_setup(handle_ipc, p, nonblock);
}

#ifdef __cplusplus
}
#endif

