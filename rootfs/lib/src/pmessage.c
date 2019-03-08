#include <pmessage.h>
#include <types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int popen(int pid, uint32_t bufSize) {
	return syscall2(SYSCALL_KOPEN, pid, (int32_t)bufSize);
}

void pclose(int id) {
	syscall1(SYSCALL_KCLOSE, id);
}

static void pring(int id) {
	syscall1(SYSCALL_KRING, id);
}

static int ppeer(int id) {
	return syscall1(SYSCALL_KPEER, id);
}

static int pwrite(int id, void* data, uint32_t size) {
	//return syscall3(SYSCALL_KWRITE, id, (int)data, size);
	int i;
	while(true) {
		i = syscall3(SYSCALL_KWRITE, id, (int)data, size);
		if(i >= 0)
			break;
		yield();
	}
	return i;
}

static int pread(int id, void* data, uint32_t size) {
	if(data == NULL || size == 0)
		return 0;

	//return syscall3(SYSCALL_KREAD, id, (int)data, size);
	int i;
	while(true) {
		i = syscall3(SYSCALL_KREAD, id, (int)data, size);
		if(i >= 0)
			break;
		yield();
	}
	return i;
}

int psend(int id, uint32_t type, void* data, uint32_t size) {
	int i = pwrite(id, &size, 4);
	if(i == 0)
		return 0;

	i = pwrite(id, &type, 4);
	if(i == 0)
		return 0;

	const char* p = (const char*)data;
	int32_t ret = size;
	while(true) {
		i = pwrite(id, (void*)p, size);
		pring(id); //give ring to reader for reading and clear buffer.

		if(i == 0)
			break;

		size -= i;
		if(size == 0)
			break;
		p += i;
	}
	return ret;
}

PackageT* precvPkg(int id) {
	uint32_t type, size;
	int i = pread(id, &size, 4);
	if(i == 0)
		return NULL;

	i = pread(id, &type, 4);
	if(i == 0)
		return NULL;

	int fromPID = ppeer(id);
	PackageT* pkg = newPackage(id, type, NULL, size, fromPID);
	if(pkg == NULL)
		return NULL;

	uint32_t sz = size;
	char* p = (char*)getPackageData(pkg);
	while(true) {
		i = pread(id, p, sz);
		if(i == 0) {
			return pkg;
		}

		sz -= i;
		if(sz == 0)
			break;

		pring(id);  //wait for more data, give ring to write proc
		p += i;
	}
	return pkg;
}

PackageT* preq(int pid, uint32_t bufSize, uint32_t type, void* data, uint32_t size, bool reply) {
	int id = popen(pid, bufSize);
	if(id < 0)
		return NULL;

	int i = psend(id, type, data, size);
	if(i == 0 || !reply) {
		pclose(id);
		return NULL;
	}
	
	PackageT* pkg = precvPkg(id);
	pclose(id);
	return pkg;
}

PackageT* proll() {
	int id = -1;
	while(true) {
		id = syscall0(SYSCALL_KREADY);
		if(id >= 0)
			break;
		yield();
	}
	
	PackageT* pkg = precvPkg(id);
	return pkg;
}
