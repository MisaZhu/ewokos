#ifndef VFS_NODE_H
#define VFS_NODE_H

#include <fsinfo.h>
#define VFS_DIR_SIZE 0xffffffff

typedef struct {
	char dev_name[DEV_NAME_MAX];
	int32_t dev_index;
	int32_t dev_serv_pid;
	uint32_t node_old;
} mount_t;

uint32_t vfs_add(uint32_t node, const char* name, uint32_t size, void* data);

int32_t vfs_del(uint32_t node);

int32_t vfs_node_full_name(uint32_t node, char* full, uint32_t len);

int32_t vfs_node_by_name(const char* fname, fs_info_t* info);

int32_t vfs_node_update(fs_info_t* info);

uint32_t vfs_mount(const char* fname, const char* devName, int32_t devIndex, bool isFile);

int32_t vfs_unmount(uint32_t node);

int32_t vfs_kid(uint32_t node, int32_t index, fs_info_t* kid);

int32_t vfs_get_mount(int32_t index, mount_t* mnt);

#endif
