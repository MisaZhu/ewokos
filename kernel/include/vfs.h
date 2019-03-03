#ifndef VFS_H
#define VFS_H

#include <fstree.h>

void vfsInit();

TreeNodeT* getNodeByFD(int32_t pid, int32_t fd);

TreeNodeT* vfsAdd(TreeNodeT* nodeTo, const char* name, uint32_t size);

int32_t vfsDel(TreeNodeT* node);

int32_t vfsNodeInfo(TreeNodeT* node, FSInfoT* info);

FSInfoT* vfsNodeKids(TreeNodeT* node, int32_t* num);

TreeNodeT* getNodeByName(const char* fname);

TreeNodeT* vfsMount(const char* fname, const char* devName, int32_t devIndex);

int32_t vfsUnmount(TreeNodeT* node);

#endif
