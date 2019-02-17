#ifndef FS_H
#define FS_H

#include <fsinfo.h>

#define KSERV_FS_NAME  "FS"

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
	FS_UNMOUNT
};

int fsOpen(const char* name);

int fsClose(int fd);

int fsInfo(int fd, FSInfoT* info);

int fsFInfo(const char* name, FSInfoT* info);

int fsChild(int fd, FSInfoT* childInfo); /*dir first child*/

int fsRead(int fd, char* buf, uint32_t size);

int fsGetch(int fd);

int fsPutch(int fd, int c);

int fsWrite(int fd, const char* buf, uint32_t size);

int fsAdd(int dirFD, const char* name);

/*return fs service process pid, -1 means not inited*/
int fsInited();

#endif
