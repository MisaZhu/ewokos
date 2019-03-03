#ifndef VFS_NODE_H
#define VFS_NODE_H

#include <fsinfo.h>

uint32_t vfsAdd(uint32_t nodeTo, const char* name, uint32_t size);

int32_t vfsDel(uint32_t node);

int32_t vfsNodeInfo(uint32_t node, FSInfoT* info);

uint32_t vfsNodeByFD(int32_t pid, int32_t fd);

uint32_t vfsNodeByName(const char* fname);

uint32_t vfsMount(const char* fname, const char* devName, int32_t devIndex);

int32_t vfsUnmount(uint32_t node);


#endif
