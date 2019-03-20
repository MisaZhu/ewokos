#ifndef VFS_NODE_H
#define VFS_NODE_H

#include <fsinfo.h>

uint32_t vfs_add(uint32_t node, const char* name, uint32_t size);

int32_t vfs_del(uint32_t node);

int32_t vfs_node_info(uint32_t node, fs_info_t* info);

uint32_t vfs_node_by_fd(int32_t fd);

uint32_t vfs_node_by_name(const char* fname);

uint32_t vfs_mount(const char* fname, const char* devName, int32_t devIndex, bool isFile);

int32_t vfs_unmount(uint32_t node);

fs_info_t* vfs_kids(uint32_t node, uint32_t* num);

#endif
