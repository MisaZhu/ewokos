#include <ipc.h>
#include <types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>

int ipc_open(int pid, uint32_t buf_size) {
	return syscall2(SYSCALL_IPC_OPEN, pid, (int32_t)buf_size);
}

void ipc_close(int id) {
	while(syscall1(SYSCALL_IPC_CLOSE, id) != 0)
		sleep(0);
}

static void ipc_ring(int id) {
	syscall1(SYSCALL_IPC_RING, id);
}

static int ipc_peer(int id) {
	return syscall1(SYSCALL_IPC_PEER, id);
}

static int ipc_write(int id, void* data, uint32_t size) {
	int i;
	while(true) {
		i = syscall3(SYSCALL_IPC_WRITE, id, (int)data, size);
		if(i >= 0)
			break;
		sleep(0);
	}
	return i;
}

static int ipc_read(int id, void* data, uint32_t size) {
	if(data == NULL || size == 0)
		return 0;
	int i;
	while(true) {
		i = syscall3(SYSCALL_IPC_READ, id, (int)data, size);
		if(i >= 0)
			break;
		sleep(0);
	}
	return i;
}

int ipc_send(int id, uint32_t type, void* data, uint32_t size) {
	int i = ipc_write(id, &size, 4);
	if(i == 0)
		return -1;

	i = ipc_write(id, &type, 4);
	if(i == 0)
		return -1;

	const char* p = (const char*)data;
	int32_t ret = size;
	while(true) {
		i = ipc_write(id, (void*)p, size);
		ipc_ring(id); //give ring to reader for reading and clear buffer.

		if(i == 0)
			break;

		size -= i;
		if(size == 0)
			break;
		p += i;
	}
	return ret;
}

package_t* ipc_recv(int id) {
	uint32_t type, size;
	int i = ipc_read(id, &size, 4);
	if(i == 0)
		return NULL;

	i = ipc_read(id, &type, 4);
	if(i == 0)
		return NULL;

	int fromPID = ipc_peer(id);
	package_t* pkg = pkg_new(id, type, NULL, size, fromPID);
	if(pkg == NULL)
		return NULL;

	uint32_t sz = size;
	char* p = (char*)get_pkg_data(pkg);
	while(true) {
		i = ipc_read(id, p, sz);
		if(i == 0) {
			return pkg;
		}

		sz -= i;
		if(sz == 0)
			break;

		ipc_ring(id);  //wait for more data, give ring to write proc
		p += i;
	}
	return pkg;
}

static package_t* ipc_req(int pid, uint32_t buf_size, uint32_t type, void* data, uint32_t size, bool reply) {
	int id = ipc_open(pid, buf_size);
	if(id < 0)
		return NULL;

	int i = ipc_send(id, type, data, size);
	if(i != (int32_t)size) {
		ipc_close(id);
		return NULL;
	}
	
	package_t* pkg = ipc_recv(id);
	ipc_close(id);
	if(!reply && pkg != NULL) {
		free(pkg);
		return NULL;
	}
	return pkg;
}

int32_t ipc_call(int32_t pid, int32_t call_id, const proto_t* input, proto_t* output) {
	if(pid < 0)
		return -1;
	package_t* pkg;
	if(input == NULL)
		pkg = ipc_req(pid, 0, call_id, NULL, 0, true);
	else
		pkg = ipc_req(pid, 0, call_id, input->data, input->size, true);

	errno = ENONE;
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR || pkg->type == PKG_TYPE_AGAIN) {
		if(pkg != NULL) {
			if(pkg->type == PKG_TYPE_AGAIN)
				errno = EAGAIN;
			free(pkg);
		}
		return -1;
	}

	if(output == NULL) {
		free(pkg);
		return 0;
	}
	proto_copy(output, get_pkg_data(pkg), pkg->size);
	free(pkg);
	return 0;
}

package_t* ipc_roll(bool block) {
	int id = -1;
	while(true) {
		id = syscall1(SYSCALL_IPC_READY, (int32_t)block);
		if(id >= 0)
			break;
		else {
			if(!block)
				return NULL;
			sleep(0);
		}
	}
	
	package_t* pkg = ipc_recv(id);
	return pkg;
}
