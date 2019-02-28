#ifndef KDEV_H
#define KDEV_H

#include "types.h"
#include "fsinfo.h"
#include "tree.h"

typedef struct {
	char name[DEV_NAME_MAX];
	bool (*mount)(int32_t servPID, int32_t index, TreeNodeT* node);
	bool (*open)(int32_t servPID, int32_t index, TreeNodeT* node, int32_t flag);
	bool (*close)(int32_t servPID, int32_t index, TreeNodeT* node);
	int32_t (*read)(int32_t servPID, int32_t index, TreeNodeT* node, char* buf, uint32_t size, int32_t seek);
	int32_t (*write)(int32_t servPID, int32_t index, TreeNodeT* node, char* buf, uint32_t size, int32_t seek);
	bool (*info)(int32_t servPID, int32_t index, TreeNodeT* node, FSInfoT* info);
	bool (*ioctl)(int32_t servPID, int32_t index, TreeNodeT* node, int32_t cmd, int32_t value);
	bool (*add)(int32_t servPID, int32_t index, TreeNodeT* node);
	bool (*del)(int32_t servPID, int32_t index, TreeNodeT* node);
} DeviceT;

void devInit();

DeviceT* devByName(const char* name);

DeviceT* devByID(int32_t id);

int32_t regDev(DeviceT* dev);

#define KDEV_INITRD "kdev.initrd"
#define KDEV_IPC "kdev.ipc"

#endif
