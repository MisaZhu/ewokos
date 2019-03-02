#ifndef VFS_H
#define VFS_H

#include <fstree.h>

void vfsInit();

TreeNodeT* getNodeByFD(int32_t fd);

TreeNodeT* vfsAdd(TreeNodeT* nodeTo, FSNodeT* info);

int32_t vfsDel(TreeNodeT* node);

int32_t vfsInfo(TreeNodeT* node, FSNodeT* info);

TreeNodeT* getNodeByFD(int32_t fd);

TreeNodeT* getNodeByName(const char* fname);

TreeNodeT* vfsMount(const char* fname, FSNodeT* info);

int32_t vfsUnmount(TreeNodeT* node);

#endif
