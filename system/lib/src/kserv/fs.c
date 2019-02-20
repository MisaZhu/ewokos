#include <kserv/fs.h>
#include <syscall.h>

int fsOpen(const char* name, int32_t flags) {
	return syscall2(SYSCALL_VFS_OPEN, (int32_t)name, flags);
}

int fsFInfo(const char* name, FSInfoT* info) {
	return syscall2(SYSCALL_VFS_FINFO, (int32_t)name, (int32_t)info);
}

int fsClose(int fd) {
	return syscall1(SYSCALL_VFS_CLOSE, fd);
}

int fsRead(int fd, char* buf, uint32_t size) {
	return syscall3(SYSCALL_VFS_READ, fd, (int32_t)buf, (int32_t)size);
}

int fsWrite(int fd, const char* buf, uint32_t size) {
	return syscall3(SYSCALL_VFS_WRITE, fd, (int32_t)buf, (int32_t)size);
}

int fsAdd(int dirFD, const char* name) {
	return syscall2(SYSCALL_VFS_ADD, dirFD, (int32_t)name);
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
	return syscall2(SYSCALL_VFS_INFO, fd, (int32_t)info);
}

FSInfoT* fsKids(int fd, int32_t *num) {
	return (FSInfoT*)syscall2(SYSCALL_VFS_KIDS, fd, (int32_t)num);
}

int fsInited() {
	return 1;//kservGetPid(KSERV_VFS_NAME);
}
