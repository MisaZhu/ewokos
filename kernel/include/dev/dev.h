#ifndef KDEV_H
#define KDEV_H

#include "types.h"
#include "fsinfo.h"
#include "tree.h"

typedef struct {
	char name[DEV_NAME_MAX];
	
	bool (*mount)(TreeNodeT* node);
	bool (*open)(TreeNodeT* node, int32_t flag);
	bool (*close)(TreeNodeT* node);
	bool (*info)(TreeNodeT* node, FSInfoT* info);
	bool (*setopt)(TreeNodeT* node, int32_t cmd, int32_t value);
	bool (*add)(TreeNodeT* node);
	bool (*del)(TreeNodeT* node);
} DeviceT;

DeviceT* devByName(const char* name);

#define KDEV_INITRD "kdev.initrd"

#endif
