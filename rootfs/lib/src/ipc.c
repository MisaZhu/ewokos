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
		yield();
}

static void ipc_ring(int id) {
	syscall1(SYSCALL_IPC_RING, id);
}

static int ipc_peer(int id) {
	return syscall1(SYSCALL_IPC_PEER, id);
}

static int ipc_write(int id, void* data, uint32_t size) {
	//return syscall3(SYSCALL_IPC_WRITE, id, (int)data, size);
	int i;
	while(true) {
		i = syscall3(SYSCALL_IPC_WRITE, id, (int)data, size);
		if(i >= 0)
			break;
		yield();
	}
	return i;
}

static int ipc_read(int id, void* data, uint32_t size) {
	if(data == NULL || size == 0)
		return 0;

	//return syscall3(SYSCALL_IPC_READ, id, (int)data, size);
	int i;
	while(true) {
		i = syscall3(SYSCALL_IPC_READ, id, (int)data, size);
		if(i >= 0)
			break;
		yield();
	}
	return i;
}

int ipc_send(int id, uint32_t type, void* data, uint32_t size) {
	int i = ipc_write(id, &size, 4);
	if(i == 0)
		return 0;

	i = ipc_write(id, &type, 4);
	if(i == 0)
		return 0;

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

package_t* ipc_req(int pid, uint32_t buf_size, uint32_t type, void* data, uint32_t size, bool reply) {
	int id = ipc_open(pid, buf_size);
	if(id < 0)
		return NULL;

	int i = ipc_send(id, type, data, size);
	if(i == 0 || !reply) {
		ipc_close(id);
		return NULL;
	}
	
	package_t* pkg = ipc_recv(id);
	ipc_close(id);
	return pkg;
}

package_t* ipc_roll() {
	int id = -1;
	while(true) {
		id = syscall0(SYSCALL_IPC_READY);
		if(id >= 0)
			break;
		yield();
	}
	
	package_t* pkg = ipc_recv(id);
	return pkg;
}
