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

static void ipc_end(void) {
	syscall0(SYS_IPC_END);
}

int ipc_disable(void) {
	while(true) {
		int res = syscall0(SYS_IPC_DISABLE);
		if(res == 0)
			break;
		sleep(0);
	}
	return 0;
}

void ipc_enable(void) {
	syscall0(SYS_IPC_ENABLE);
}

static proto_t* ipc_get_info(uint32_t ipc_id, int32_t* pid, int32_t* call_id) {
	return (proto_t*)syscall3(SYS_IPC_GET_ARG, ipc_id, (int32_t)pid, (int32_t)call_id);
}

inline int ipc_call(int to_pid, int call_id, const proto_t* ipkg, proto_t* opkg) {
	if(to_pid < 0)
		return -1;
	int ipc_id = 0;
	while(true) {
		if(opkg == NULL)
			call_id |= IPC_NON_RETURN;
		ipc_id = syscall3(SYS_IPC_CALL, (int32_t)to_pid, (int32_t)call_id, (int32_t)ipkg);

		if(ipc_id == -1)
			continue;
		if(ipc_id == 0)
			return -1;
		break;
	}

	if(opkg == NULL)
		return 0;

	PF->clear(opkg);
	while(true) {
		int res = syscall3(SYS_IPC_GET_RETURN, to_pid, (int32_t)ipc_id, (int32_t)opkg);
		if(res != -1)  //not retry
			return res;
	}
	return 0;
}

inline int ipc_call_wait(int to_pid, int call_id, const proto_t* ipkg, proto_t* opkg) {
	if(opkg != NULL)
		return ipc_call(to_pid, call_id, ipkg, opkg);
	
	proto_t out;
	PF->init(&out);
	int res = ipc_call(to_pid, call_id, ipkg, &out);
	PF->clear(&out);
	return res;
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
static ipc_handled_t _ipc_handled;

static void handle_ipc(uint32_t ipc_id, void* p) {
	int32_t pid, cmd;

	proto_t* in = ipc_get_info(ipc_id, &pid, &cmd);
	if((cmd & IPC_NON_RETURN) != 0) { //no return
		_ipc_serv_handle(pid, (cmd & IPC_NON_RETURN_MASK), in, NULL, p);
		proto_free(in);
	}
	else {
		proto_t out;
		PF->init(&out);
		_ipc_serv_handle(pid, cmd, in, &out, p);
		proto_free(in);
		ipc_set_return(ipc_id, &out);
		PF->clear(&out);
	}
	if(_ipc_handled != NULL)
		_ipc_handled(p);
	ipc_end();
}

int ipc_serv_run(ipc_serv_handle_t handle, ipc_handled_t handled, void* p, int flags) {
	_ipc_serv_handle = handle;
	_ipc_handled = handled;

	proc_ready_ping();
	return ipc_setup(handle_ipc, p, flags);
}

#ifdef __cplusplus
}
#endif

