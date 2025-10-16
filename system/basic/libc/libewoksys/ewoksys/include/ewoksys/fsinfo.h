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

#define FS_TYPE_ANNOUNIMOUS 0x80000000
#define FS_TYPE_MASK        0x7fffffff

enum {
	FS_CMD_NONE = 0,
	FS_CMD_OPEN,
	FS_CMD_STAT,
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
	FS_CMD_DEV_CNTL,
	FS_CMD_SET,
	FS_CMD_CMD
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
	uint16_t	mode;		/* File mode */
	uint16_t	uid;		/* Owner Uid */
	uint16_t	gid;		/* Group Id */
	uint32_t	size;		/* Size in bytes */
	uint32_t	atime;	/* Access time */
	uint32_t	ctime;	/* Creation time */
	uint32_t	mtime;	/* Modification time */
	uint16_t	links_count;	/* Links count */
} node_stat_t;

#define FS_STATE_CHANGED 0x01
#define FS_STATE_BUSY_R  0x02
#define FS_STATE_BUSY_W  0x04
#define FS_STATE_BUSY_X  0x08

typedef struct {
	uint32_t type;
	uint32_t node;
	int32_t  mount_pid;
	char     name[FS_NODE_NAME_MAX];

	node_stat_t stat;
	uint32_t data;
	uint32_t state;
} fsinfo_t;


#ifdef __cplusplus 
}
#endif

#endif

