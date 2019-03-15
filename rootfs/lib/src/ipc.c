#include <ipc.h>
#include <types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <kstring.h>
#include <unistd.h>

int ipcOpen(int pid, uint32_t bufSize) {
	return syscall2(SYSCALL_IPC_OPEN, pid, (int32_t)bufSize);
}

void ipcClose(int id) {
	while(syscall1(SYSCALL_IPC_CLOSE, id) != 0)
		yield();
}

static void ipcRing(int id) {
	syscall1(SYSCALL_IPC_RING, id);
}

static int ipcPeer(int id) {
	return syscall1(SYSCALL_IPC_PEER, id);
}

static int ipcWrite(int id, void* data, uint32_t size) {
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

static int ipcRead(int id, void* data, uint32_t size) {
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

int ipcSend(int id, uint32_t type, void* data, uint32_t size) {
	int i = ipcWrite(id, &size, 4);
	if(i == 0)
		return 0;

	i = ipcWrite(id, &type, 4);
	if(i == 0)
		return 0;

	const char* p = (const char*)data;
	int32_t ret = size;
	while(true) {
		i = ipcWrite(id, (void*)p, size);
		ipcRing(id); //give ring to reader for reading and clear buffer.

		if(i == 0)
			break;

		size -= i;
		if(size == 0)
			break;
		p += i;
	}
	return ret;
}

PackageT* ipcRecv(int id) {
	uint32_t type, size;
	int i = ipcRead(id, &size, 4);
	if(i == 0)
		return NULL;

	i = ipcRead(id, &type, 4);
	if(i == 0)
		return NULL;

	int fromPID = ipcPeer(id);
	PackageT* pkg = newPackage(id, type, NULL, size, fromPID);
	if(pkg == NULL)
		return NULL;

	uint32_t sz = size;
	char* p = (char*)getPackageData(pkg);
	while(true) {
		i = ipcRead(id, p, sz);
		if(i == 0) {
			return pkg;
		}

		sz -= i;
		if(sz == 0)
			break;

		ipcRing(id);  //wait for more data, give ring to write proc
		p += i;
	}
	return pkg;
}

PackageT* ipcReq(int pid, uint32_t bufSize, uint32_t type, void* data, uint32_t size, bool reply) {
	int id = ipcOpen(pid, bufSize);
	if(id < 0)
		return NULL;

	int i = ipcSend(id, type, data, size);
	if(i == 0 || !reply) {
		ipcClose(id);
		return NULL;
	}
	
	PackageT* pkg = ipcRecv(id);
	ipcClose(id);
	return pkg;
}

PackageT* ipcRoll() {
	int id = -1;
	while(true) {
		id = syscall0(SYSCALL_IPC_READY);
		if(id >= 0)
			break;
		yield();
	}
	
	PackageT* pkg = ipcRecv(id);
	return pkg;
}
