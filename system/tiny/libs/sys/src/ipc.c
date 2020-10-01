#include <sys/ipc.h>
#include <sys/syscall.h>
#include <sys/core.h>
#include <sys/proc.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


static int ipc_setup(ipc_handle_t handle, void* p, int flags) {
	return syscall3(SYS_IPC_SETUP, (int32_t)handle, (int32_t)p, (int32_t)flags);
}

static int ipc_set_return(uint32_t ipc_id, const proto_t* pkg) {
	syscall2(SYS_IPC_SET_RETURN, ipc_id, (int32_t)pkg);
	return 0;
}

static void ipc_end(uint32_t ipc_id) {
	syscall1(SYS_IPC_END, ipc_id);
}

void ipc_lock(void) {
	syscall0(SYS_IPC_LOCK);
}

void ipc_unlock(void) {
	syscall0(SYS_IPC_UNLOCK);
}

static proto_t* ipc_get_info(uint32_t ipc_id, int32_t* pid, int32_t* call_id) {
	return (proto_t*)syscall3(SYS_IPC_GET_ARG, ipc_id, (int32_t)pid, (int32_t)call_id);
}

int ipc_call(int to_pid, int call_id, const proto_t* ipkg, proto_t* opkg) {
	if(to_pid < 0)
		return -1;

	int ipc_id = 0;
	int res = 0;
	while(true) {
		ipc_id = syscall3(SYS_IPC_CALL, (int32_t)to_pid, (int32_t)call_id, (int32_t)ipkg);
		if(ipc_id == -1) {
			sleep(0); //retry	
			continue;
		}
		if(ipc_id == 0) {
			return -1;
		}
		break;
	}

	if(opkg != NULL)
		PF->clear(opkg);

	while(true) {
		res = syscall2(SYS_IPC_GET_RETURN, (int32_t)ipc_id, (int32_t)opkg);
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
	PF->init(&out);

	PF->init(&in)->adds(&in, ipc_serv_id);
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
	PF->init(&out);

	PF->init(&in)->adds(&in, ipc_serv_id);
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
	PF->init(&out);

	PF->init(&in)->adds(&in, ipc_serv_id);
	if(ipc_call(core_pid, CORE_CMD_IPC_SERV_UNREG, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	PF->clear(&in);
	PF->clear(&out);
	return res;
}

static ipc_serv_handle_t _ipc_serv_handle;

static void handle_ipc(uint32_t ipc_id, void* p) {
	int32_t pid, cmd;
	proto_t out;
	PF->init(&out);

	proto_t* in = ipc_get_info(ipc_id, &pid, &cmd);
	_ipc_serv_handle(pid, cmd, in, &out, p);
	proto_free(in);

	ipc_set_return(ipc_id, &out);
	PF->clear(&out);
	ipc_end(ipc_id);
}

int ipc_serv_run(ipc_serv_handle_t handle, void* p, int flags) {
	_ipc_serv_handle = handle;

	proc_ready_ping();
	return ipc_setup(handle_ipc, p, flags);
}

#ifdef __cplusplus
}
#endif

