#ifndef FS_H
#define FS_H

#include <types.h>

#define KSERV_FS_NAME  "FS"

typedef enum {
	FS_OPEN = 0,
	FS_CLOSE,
	FS_WRITE,
	FS_READ,
	FS_INFO,
	FS_CHILD,
	FS_NEXT,
} FSCmdT;

#define FS_TYPE_DIR 0x0
#define FS_TYPE_FILE 0x01

typedef struct FSInfo {
	uint32_t size;
	uint32_t type;
	char name[FNAME_MAX];
} FSInfoT;

int fsOpen(const char* name);

int fsClose(int fd);

int fsInfo(int fd, FSInfoT* info);

int fsChild(int fd, FSInfoT* childInfo); /*dir first child*/

int fsRead(int fd, char* buf, uint32_t size);

int fsGetch(int fd);

int fsPutch(int fd, int c);

int fsWrite(int fd, const char* buf, uint32_t size);

/*return fs service process pid, -1 means not inited*/
int fsInited();

#endif
