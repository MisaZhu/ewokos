#ifndef FS_H
#define FS_H

#include <fsinfo.h>
#include <dev.h>

#define DEV_VFS  "vfs"
#define KSERV_VFS_NAME  devGetServName(DEV_VFS)

enum {
	FS_OPEN = 0,
	FS_CLOSE,
	FS_WRITE,
	FS_READ,
	FS_INFO,
	FS_FINFO,
	FS_ADD,
	FS_CHILD,
	FS_NEXT,
	FS_MOUNT,
	FS_UNMOUNT,
	FS_FDEV,
	FS_DEV
};

int fsOpen(const char* name, int32_t flags);

int fsClose(int fd);

int fsInfo(int fd, FSInfoT* info);

int fsFInfo(const char* name, FSInfoT* info);

FSInfoT* fsKids(int fd, int32_t *num);

int fsRead(int fd, char* buf, uint32_t size);

int fsGetch(int fd);

int fsPutch(int fd, int c);

int fsWrite(int fd, const char* buf, uint32_t size);

int fsAdd(int dirFD, const char* name);

/*return fs service process pid, -1 means not inited*/
int fsInited();

#endif
