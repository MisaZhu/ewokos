#include <pmessage.h>
#include <syscall.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MSG_RETRY 128

PackageT* newPackage(uint32_t type, void* data, uint32_t size) {
	PackageT* pkg = (PackageT*)malloc(sizeof(PackageT) + size);
	if(pkg == NULL)
		return NULL;

	pkg->id = -1;
	pkg->pid = -1;
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

static int psendPkg(int id, int pid, PackageT* pkg) {
	return syscall3(SYSCALL_SEND_MSG, id, pid, (int)pkg);
}

int psend(int id, int pid, uint32_t type, void* data, uint32_t size) {
	PackageT* pkg = newPackage(type, data, size);
	int ret = psendPkg(id, pid, pkg);
	freePackage(pkg);
	return ret;
}

PackageT* precv(int id) {
	return (PackageT*)syscall1(SYSCALL_READ_MSG, id);
}

int psendSync(int id, int pid, uint32_t type, void* data, uint32_t size) {
	for(int i=0; i<MSG_RETRY; i++)  {
		int ret = psend(id, pid, type, data, size);
		if(ret >= 0)
			return ret;
		else
			yield();
	}
	return -1;
}

PackageT* precvSync(int id) {
	for(int i=0; i<MSG_RETRY; i++)  {
		PackageT* ret = precv(id);
		if(ret != 0)
			return ret;
		else
			yield();
	}
	return 0;
}

PackageT* preq(int pid, uint32_t type, void* data, uint32_t size) {
	int id = psendSync(-1, pid, type, data, size);
	return precvSync(id);
}
