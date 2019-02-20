#ifndef VFS_H
#define VFS_H

#include "fsinfo.h"
#include "tree.h"
#include "dev/dev.h"

typedef struct {
	DeviceT* device;	
  uint32_t index;
  struct TreeNode* to;
} MountT;

void vfsInit();

int32_t vfsMount(const char* to, const char* deviceName, uint32_t index);

int32_t vfsOpen(const char* name, int32_t flags);

int32_t vfsClose(int32_t fd);

int32_t vfsRead(int32_t fd, void* buf, uint32_t size, int32_t geek);

int32_t vfsWrite(int32_t fd, void* buf, uint32_t size, int32_t geek);

int32_t vfsFInfo(const char* name, FSInfoT* info);

int32_t vfsNodeInfo(TreeNodeT* node, FSInfoT* info);

int32_t vfsInfo(int32_t fd, FSInfoT* info);

int32_t vfsIoctl(int32_t fd, int32_t cmd, int32_t value);

int32_t vfsAdd(int32_t fd, const char* name);

int32_t vfsDel(int32_t fd, const char* name);

/*return value has to be free later*/
FSInfoT* vfsFKids(const char* name, int32_t* num);

/*return value has to be free later*/
FSInfoT* vfsKids(int32_t fd, int32_t* num);

#endif
