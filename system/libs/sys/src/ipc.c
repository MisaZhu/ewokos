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

int ipc_call(int to_pid,  const proto_t* ipkg, proto_t* opkg) {
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

int ipc_server(ipc_handle_t handle, void* p) {
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
