#include <sys/ipc.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <rawdata.h>
#include <unistd.h>

static int ipc_send_raw(int pid, void* data, uint32_t size, int32_t id) {
	rawdata_t rawdata;
	rawdata.data = data;
	rawdata.size = size;
	int res = syscall3(SYS_SEND_MSG, (int32_t)pid, (int32_t)&rawdata, id);
	sleep(0);
	return res;
}

static int ipc_get_raw(int* from_pid,  rawdata_t* data, int32_t id, uint8_t block) {
	if(block == 1) 
		return syscall3(SYS_GET_MSG, (int32_t)from_pid, (int32_t)data, id);
	return syscall3(SYS_GET_MSG_NBLOCK, (int32_t)from_pid, (int32_t)data, id);
}

int ipc_send(int to_pid, const proto_t* pkg, int32_t id) {
	if(to_pid < 0 || pkg == NULL)
		return -1;
	return ipc_send_raw(to_pid, pkg->data, pkg->size, id);
}

int ipc_get(int* from_pid,  proto_t* pkg, int32_t id, uint8_t block) {
	rawdata_t rawdata = { 0 };
	id = ipc_get_raw(from_pid, &rawdata, id, block);
	if(id < 0)
		return -1;

	if(pkg != NULL) {
		proto_clear(pkg);
		proto_init(pkg, rawdata.data, rawdata.size);
		pkg->id = id;
		pkg->read_only = 0;
	}
	return id;	
}

static int ipc_recv(int* from_pid,  proto_t* pkg, int32_t id) {
	if(pkg == NULL)
		return -1;

	while(1) {
		int res = ipc_get(from_pid, pkg, id, 1);
		if(res >= 0)
			return 0;
	}
	return -1;
}

int ipc_msg_call(int to_pid,  const proto_t* ipkg, proto_t* opkg) {
	if(to_pid < 0 || ipkg == NULL)
		return -1;
	int32_t id = ipc_send(to_pid, ipkg, -1);
	if(id < 0) {
		return -1;
	}
	if(opkg == NULL)
		return 0;

	if(ipc_recv(NULL, opkg, id) != 0) {
		return -1;
	}
	return 0;
}

int ipc_server(ipc_msg_handle_t handle, void* p) {
	proto_t pkg;
	proto_init(&pkg, NULL, 0);
	while(1) {
		int pid;
		if(ipc_get(&pid, &pkg, -1, 1) >= 0) {
			handle(pid, &pkg, p);
			proto_clear(&pkg);
		}
	}
}

int ipc_setup(ipc_handle_t handle, void* p) {
	return syscall2(SYS_IPC_SETUP, (int32_t)handle, (int32_t)p);
}

int ipc_return(const proto_t* pkg) {
	syscall1(SYS_IPC_RETURN, (int32_t)pkg);
	return 0;
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
		res = syscall2(SYS_IPC_GET_RETURN, (int32_t)to_pid, opkg);
		if(res == 0)
			break;
		if(res == -2)
			return -1;
		sleep(0); //retry	
	}
	return 0;
}

