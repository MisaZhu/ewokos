#include <pmessage.h>
#include <types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

PackageT* newPackage(int32_t id, uint32_t type, void* data, uint32_t size, int32_t pid) {
	PackageT* pkg = (PackageT*)malloc(sizeof(PackageT) + size);
	if(pkg == NULL)
		return NULL;

	pkg->id = id;
	pkg->pid = pid;
	pkg->size = 0;
	pkg->type = type;

	void* p = getPackageData(pkg);
	if(size > 0 && data != NULL)
		memcpy(p, data, size);

	pkg->size = size;
	return pkg;
}

void freePackage(PackageT* pkg) {
	if(pkg != NULL)
		free(pkg);
}

int popen(int pid) {
	return syscall1(SYSCALL_KOPEN, pid);
}

void pclose(int id) {
	syscall1(SYSCALL_KCLOSE, id);
}

int pwrite(int id, void* data, uint32_t size) {
	int i;
	while(true) {
		i = syscall3(SYSCALL_KWRITE, id, (int)data, size);
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
		if(i == 0)
			break;
		yield();

		size -= i;
		if(size == 0)
			break;
		p += i;
	}
	return ret;
}

int pread(int id, void* data, uint32_t size) {
	if(data == NULL || size == 0)
		return 0;

	int i;
	while(true) {
		i = syscall3(SYSCALL_KREAD, id, (int)data, size);
		if(i >= 0)
			break;
		yield();
	}
	return i;
}

PackageT* precvPkg(int id) {
	uint32_t type, size;
	int i = pread(id, &size, 4);
	if(i == 0)
		return NULL;

	i = pread(id, &type, 4);
	if(i == 0)
		return NULL;

	int fromPID = pgetPidW(id);
	PackageT* pkg = newPackage(id, type, NULL, size, fromPID);
	if(pkg == NULL)
		return NULL;

	uint32_t sz = size;
	void* data = malloc(size);
	if(data == NULL) {
		free(pkg);
		return NULL;
	}

	char* p = (char*)getPackageData(pkg);
	while(true) {
		i = pread(id, p, sz);
		if(i == 0) {
			return NULL;
		}

		sz -= i;
		if(sz == 0)
			break;
		p += i;
	}
	return pkg;
}

int pgetPidR(int32_t id) {
	return syscall1(SYSCALL_KGETPID_R, id);
}

int pgetPidW(int32_t id) {
	return syscall1(SYSCALL_KGETPID_W, id);
}

PackageT* preq(int pid, uint32_t type, void* data, uint32_t size, bool reply) {
	int id = popen(pid);
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
