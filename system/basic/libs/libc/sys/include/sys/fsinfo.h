#ifndef FS_INFO_H
#define FS_INFO_H

#include <stdint.h>
#ifdef __cplusplus 
extern "C" { 
#endif

#define FS_MOUNT_MAX 32
#define FS_NODE_NAME_MAX 64
#define FS_FULL_NAME_MAX 512

enum {
	FS_TYPE_DIR = 0,
	FS_TYPE_FILE,
	FS_TYPE_LINK,
	FS_TYPE_PIPE,
	FS_TYPE_CHAR,
	FS_TYPE_BLOCK,
	FS_TYPE_UNKNOWN
};

enum {
	FS_CMD_NONE = 0,
	FS_CMD_OPEN,
	FS_CMD_CREATE,
	FS_CMD_CLOSE,
	FS_CMD_READ,
	FS_CMD_WRITE,
	FS_CMD_READ_BLOCK,
	FS_CMD_WRITE_BLOCK,
	FS_CMD_SEEK,
	FS_CMD_DMA,
	FS_CMD_CNTL,
	FS_CMD_DUP,
	FS_CMD_UNLINK,
	FS_CMD_CLEAR_BUFFER,
	FS_CMD_FLUSH,
	FS_CMD_INTERRUPT,
	FS_CMD_DEV_CNTL
};

enum {
	DEV_CNTL_REFRESH = 0x10000
};

typedef struct {
	int32_t pid;
	uint32_t org_node;
	char org_name[FS_FULL_NAME_MAX];
} mount_t;

typedef struct {
	uint32_t type;
	uint32_t node;
	uint32_t size;
	uint32_t uid;
	uint32_t gid;
	int32_t  mount_pid;
	char     name[FS_NODE_NAME_MAX];

	uint32_t data;
} fsinfo_t;

#ifdef __cplusplus 
}
#endif

#endif

