#include <vfs/fs.h>
#include <pmessage.h>
#include <kstring.h>
#include <syscall.h>
#include <stdlib.h>
#include <proto.h>
#include <vfs/vfs.h>
#include <unistd.h>

#define FS_BUF_SIZE (16*KB)

int fsOpen(const char* name, int32_t flags) {
	uint32_t node = vfsNodeByName(name);
	if(node == 0) 
		return -1;

	FSInfoT info;
	if(vfsNodeInfo(node, &info) != 0)
		return -1;
	
	int32_t fd = syscall3(SYSCALL_PFILE_OPEN, getpid(), (int32_t)node, flags);
	if(fd < 0)
		return -1;

	if(info.devServPid == 0) 
		return fd;

	ProtoT* proto = protoNew(NULL, 0);
	protoAddInt(proto, (int32_t)node);
	protoAddInt(proto, flags);

	PackageT* pkg = preq(info.devServPid, 0, FS_OPEN, proto->data, proto->size, true);
	protoFree(proto);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}
	free(pkg);
	return fd;
}

int fsClose(int fd) {
	uint32_t node = vfsNodeByFD(getpid(), fd);
	if(node == 0) 
		return -1;

	FSInfoT info;
	if(vfsNodeInfo(node, &info) != 0)
		return -1;
	
	if(info.devServPid > 0) 
		preq(info.devServPid, 0, FS_CLOSE, (void*)&fd, 4, false);
	return syscall1(SYSCALL_PFILE_CLOSE, fd);
}

int fsRead(int fd, char* buf, uint32_t size) {
	int32_t pid = getpid();
	uint32_t node = vfsNodeByFD(pid, fd);
	if(node == 0) 
		return -1;
	uint32_t bufSize = size >= FS_BUF_SIZE ? FS_BUF_SIZE : 0;

	FSInfoT info;
	if(vfsNodeInfo(node, &info) != 0)
		return -1;

	int seek = syscall2(SYSCALL_PFILE_GET_SEEK, pid, fd);
	if(seek < 0)
		return -1;

	ProtoT* proto = protoNew(NULL, 0);
	protoAddInt(proto, node);
	protoAddInt(proto, size);
	protoAddInt(proto, seek);
	PackageT* pkg = preq(info.devServPid, bufSize, FS_READ, proto->data, proto->size, true);
	protoFree(proto);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = pkg->size;
	if(sz == 0) {
		free(pkg);
		return 0;
	}
	
	memcpy(buf, getPackageData(pkg), sz);
	free(pkg);

	seek += sz;
	syscall3(SYSCALL_PFILE_SEEK, pid, fd, seek);
	return sz;
}

int fsCtrl(int fd, int32_t cmd, void* input, uint32_t isize, void* output, uint32_t osize) {
	uint32_t node = vfsNodeByFD(getpid(), fd);
	if(node == 0) 
		return -1;

	FSInfoT info;
	if(vfsNodeInfo(node, &info) != 0)
		return -1;

	ProtoT* proto = protoNew(NULL, 0);
	protoAddInt(proto, node);
	protoAddInt(proto, cmd);
	protoAdd(proto, input, isize);
	PackageT* pkg = preq(info.devServPid, 0, FS_CTRL, proto->data, proto->size, true);
	protoFree(proto);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	if(output == NULL || osize == 0) {
		free(pkg);
		return 0;
	}
	uint32_t sz = pkg->size;

	if(sz > osize)
		sz = osize;
	memcpy(output, getPackageData(pkg), sz);
	free(pkg);
	return 0;
}

int fsWrite(int fd, const char* buf, uint32_t size) {
	uint32_t node = vfsNodeByFD(getpid(), fd);
	if(node == 0) 
		return -1;
	uint32_t bufSize = size >= FS_BUF_SIZE ? FS_BUF_SIZE : 0;

	FSInfoT info;
	if(vfsNodeInfo(node, &info) != 0)
		return -1;

	ProtoT* proto = protoNew(NULL, 0);
	protoAddInt(proto, node);
	protoAdd(proto, (void*)buf, size);
	PackageT* pkg = preq(info.devServPid, bufSize, FS_WRITE, proto->data, proto->size, true);
	protoFree(proto);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL) free(pkg);
		return -1;
	}

	int sz = *(int*)getPackageData(pkg);
	free(pkg);
	return sz;
}

int fsAdd(int fd, const char* name) {
	uint32_t node = vfsNodeByFD(getpid(), fd);
	if(node == 0) 
		return -1;

	FSInfoT info;
	if(vfsNodeInfo(node, &info) != 0)
		return -1;
	
	int size = strlen(name);
	if(size == 0)
		return -1;

	if(info.devServPid == 0) {
		return vfsAdd(vfsNodeByFD(getpid(), fd), name, 0);
	}

	ProtoT* proto = protoNew(NULL, 0);
	protoAddInt(proto, node);
	protoAddStr(proto, name);
	PackageT* pkg = preq(info.devServPid, 0, FS_ADD, proto->data, proto->size, true);
	protoFree(proto);

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

int fsFInfo(const char* name, FSInfoT* info) {
	uint32_t node = vfsNodeByName(name);
	return vfsNodeInfo(node, info);
}

int fsInfo(int fd, FSInfoT* info) {
	uint32_t node = vfsNodeByFD(getpid(), fd);
	return vfsNodeInfo(node, info);
}

FSInfoT* fsKids(int fd, uint32_t *num) {
	uint32_t node = vfsNodeByFD(getpid(), fd);
	if(node == 0)
		return NULL;
	return (FSInfoT*)syscall2(SYSCALL_VFS_KIDS, (int32_t)node, (int32_t)num);
}

int fsInited() {
	return syscall1(SYSCALL_KSERV_GET, (int)"dev.initfs");
}
