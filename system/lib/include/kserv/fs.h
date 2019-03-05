#ifndef FS_H
#define FS_H

#include <fsinfo.h>

enum {
	FS_OPEN = 0,
	FS_CLOSE,
	FS_WRITE,
	FS_READ,
	FS_CTRL,
	FS_ADD
};

int fsOpen(const char* name, int32_t flags);

int fsClose(int fd);

int fsInfo(int fd, FSInfoT* info);

int fsFInfo(const char* name, FSInfoT* info);

FSInfoT* fsKids(int fd, uint32_t *num);

int fsRead(int fd, char* buf, uint32_t size);

int fsCtrl(int fd, int32_t cmd, void* input, uint32_t isize, void* output, uint32_t osize);

int fsGetch(int fd);

int fsPutch(int fd, int c);

int fsWrite(int fd, const char* buf, uint32_t size);

int fsAdd(int dirFD, const char* name);

/*return fs service process pid, -1 means not inited*/
int fsInited();

#endif
