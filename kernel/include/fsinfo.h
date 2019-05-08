#ifndef FSINFO_H
#define FSINFO_H

#include <types.h>

#define FS_TYPE_DIR 0x0
#define FS_TYPE_FILE 0x01

typedef struct {
	uint32_t id; //vfs treeNode id
	uint32_t node;  //node of vfstree
	uint32_t size; //file size or children num of dir
	uint32_t type; //file or dir
	int32_t owner; //user owner
	uint32_t dev_index; //index for same type device
	int32_t dev_serv_pid; //device kernel service proc id
	void* data;
} fs_info_t;

#endif
