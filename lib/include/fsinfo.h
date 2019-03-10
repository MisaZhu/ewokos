#ifndef FSINFO_H
#define FSINFO_H

#include <types.h>

#define FS_TYPE_DIR 0x0
#define FS_TYPE_FILE 0x01

typedef struct FSInfo {
	uint32_t id; //vfs treeNode id
	uint32_t node;  //node of vfstree
	uint32_t size; //file size or children num of dir
	uint32_t type; //file or dir
	int32_t owner; //user owner
	char name[NAME_MAX]; //node name
	char devName[DEV_NAME_MAX]; //device name
	uint32_t devIndex; //index for same type device
	int32_t devServPid; //device kernel service proc id
} FSInfoT;

typedef struct FSNode {
	char name[NAME_MAX];
	int32_t mount;
	uint32_t size;
	uint32_t type;
	int32_t owner;
} FSNodeT;

#define FSN(n) ((FSNodeT*)((n)->data))

#endif
