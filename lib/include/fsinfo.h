#ifndef FSINFO_H
#define FSINFO_H

#include <types.h>

#define FS_TYPE_DIR 0x0
#define FS_TYPE_FILE 0x01

typedef struct FSInfo {
	uint32_t id; //vfs treeNode id
	uint32_t size;
	uint32_t type;
	uint32_t owner;
	char name[NAME_MAX];
	char device[DEV_NAME_MAX];
	uint32_t index; //index for same type device
} FSInfoT;

#define FS_FLAG_MNT_ROOT 0x01
typedef struct FSNode {
	char name[NAME_MAX];
	int32_t mount;
	uint32_t type;
	uint32_t flags;
	uint32_t owner;
} FSNodeT;

#define FSN(n) ((FSNodeT*)((n)->data))

#endif
