#ifndef FS_H
#define FS_H

#include <types.h>

#define KSERV_FS_NAME  "FS"

extern int _fsPid;

typedef enum {
	FS_OPEN = 0,
	FS_CLOSE,
	FS_WRITE,
	FS_READ,

} FSCmdT;

int fsOpen(const char* name);

int fsClose(int fd);

int fsRead(int fd, char* buf, uint32_t size);

void fsInit(int ksPid);

/*return fs service process pid, -1 means not inited*/
int fsInited();

#endif
