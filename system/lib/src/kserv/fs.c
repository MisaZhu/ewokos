#include <kserv/kserv.h>
#include <kserv/fs.h>
#include <proto.h>
#include <pmessage.h>
#include <stdlib.h>
#include <kstring.h>
#include <syscall.h>

static int fsDev(const char* name, char* dev) {
	if(name == NULL || name[0] == 0)
		return -1;
	int pid = kservGetPid(KSERV_VFS_NAME);

	PackageT* pkg = preq(pid, FS_FDEV, (void*)name, strlen(name)+1, true);
	if(pkg == NULL)	
		return -1;

	strncpy(dev, (const char*)getPackageData(pkg), DEV_NAME_MAX);
	free(pkg);
	return 0;
}

static int fsDevFD(int fd, char* dev) {
	if(fd < 0)
		return -1;
	int pid = kservGetPid(KSERV_VFS_NAME);

	PackageT* pkg = preq(pid, FS_DEV, (void*)&fd, 4, true);
	if(pkg == NULL)	
		return -1;

	strncpy(dev, (const char*)getPackageData(pkg), DEV_NAME_MAX);
	free(pkg);
	return 0;
}

static int fsDevServPidFD(int fd) {
	if(fd < 0)
		return -1;

	char dev[DEV_NAME_MAX];
	if(fsDevFD(fd, dev) < 0)
		strncpy(dev, DEV_VFS, DEV_NAME_MAX);
	return kservGetPid(devGetServName(dev));
}

static int fsDevServPid(const char* name) {
	char dev[DEV_NAME_MAX];
	if(fsDev(name, dev) < 0)
		strncpy(dev, DEV_VFS, DEV_NAME_MAX);
	return kservGetPid(devGetServName(dev));
}

int fsOpen(const char* name) {
	int pid = fsDevServPid(name);
	if(pid < 0)
		return -1;

	int fd = -1;
	PackageT* pkg = preq(pid, FS_OPEN, (void*)name, strlen(name)+1, true);
	if(pkg == NULL)	
		return -1;

	fd = *(int*)getPackageData(pkg);
	free(pkg);
	return fd;
}

int fsFInfo(const char* name, FSInfoT* info) {
	int pid = fsDevServPid(name);
	if(pid < 0)
		return -1;

	PackageT* pkg = preq(pid, FS_FINFO, (void*)name, strlen(name)+1, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	
	memcpy(info, getPackageData(pkg), sizeof(FSInfoT));
	free(pkg);
	return 0;
}


int fsClose(int fd) {
	int pid = fsDevServPidFD(fd);
	if(pid < 0)
		return -1;

	preq(pid, FS_CLOSE, (void*)&fd, 4, false);
	return 0;
}

int fsRead(int fd, char* buf, uint32_t size) {
	int pid = fsDevServPidFD(fd);
	if(pid < 0)
		return -1;

	ProtoT proto;	
	protoInit(&proto, NULL, 0);
	protoAddInt(&proto, fd);
	protoAddInt(&proto, size);

	PackageT* pkg = preq(pid, FS_READ, proto.data, proto.size, true);
	protoFree(&proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = pkg->size;
	if(sz > 0)	
		memcpy(buf, getPackageData(pkg), sz);

	free(pkg);
	return sz;
}

int fsWrite(int fd, const char* buf, uint32_t size) {
	int pid = fsDevServPidFD(fd);
	if(pid < 0)
		return -1;
	
	char *req = (char*)malloc(size + 8);
	memcpy(req, &fd, 4);
	memcpy(req+4, &size, 4);
	memcpy(req+8, buf, size);

	PackageT* pkg = preq(pid, FS_WRITE, req, size+8, true);
	free(req);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = *(int*)getPackageData(pkg);
	free(pkg);
	return sz;
}

int fsAdd(int dirFD, const char* name) {
	int pid = fsDevServPidFD(dirFD);
	if(pid < 0)
		return -1;

	int size = strlen(name);
	if(dirFD < 0 || size == 0)
		return -1;
	
	char *req = (char*)malloc(size + 8);
	memcpy(req, &dirFD, 4);
	memcpy(req+4, &size, 4);
	memcpy(req+8, name, size);

	PackageT* pkg = preq(pid, FS_ADD, req, size+8, true);
	free(req);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = *(int*)getPackageData(pkg);
	free(pkg);
	return sz;
}


int fsGetch(int fd) {
	char buf[1];
	if(fsRead(fd, buf, 1) != 1)
		return 0;
	return buf[0];
}

int fsPutch(int fd, int c) {
	char buf[1];
	buf[0] = (char)c;
	return fsWrite(fd, buf, 1);
}

int fsInfo(int fd, FSInfoT* info) {
	int pid = fsDevServPidFD(fd);
	if(pid < 0)
		return -1;
	
	PackageT* pkg = preq(pid, FS_INFO, &fd, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL)	free(pkg);
		return -1;
	}
	
	memcpy(info, getPackageData(pkg), sizeof(FSInfoT));
	free(pkg);
	return 0;
}

int fsChild(int fd, FSInfoT* child) {
	int pid = fsDevServPidFD(fd);
	if(pid < 0 || child == NULL)
		return -1;
	
	PackageT* pkg = preq(pid, FS_CHILD, &fd, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL)	free(pkg);
		return -1;
	}
	
	memcpy(child, getPackageData(pkg), sizeof(FSInfoT));
	free(pkg);
	return 0;
}

int fsInited() {
	return kservGetPid(KSERV_VFS_NAME);
}
