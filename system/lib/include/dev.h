#ifndef DEV_H
#define DEV_H

#define DEV_NAME_MAX 127

#include "fsinfo.h"
#include "tree.h"

enum {
	DEV_MOUNT = 0,
	DEV_UNMOUNT,
	DEV_OPEN,
	DEV_CLOSE,
	DEV_WRITE,
	DEV_READ,
	DEV_INFO,
	DEV_SETOPT
};

typedef struct {
	void (*mount)(TreeNodeT*);
	void (*unmount)(TreeNodeT*);
  bool (*open)(TreeNodeT*);
  int (*read)(TreeNodeT*, int, char*, uint32_t);
  int (*write)(TreeNodeT*, int, const char*, uint32_t);
  int (*info)(TreeNodeT*, FSInfoT* info);
  int (*setOpt)(TreeNodeT*, int cmd, int value);
  void (*close)(TreeNodeT*);

	char name[DEV_NAME_MAX+1];
} DeviceT;

void devInit(DeviceT* dev);

const char* devGetServName(DeviceT* dev);

bool devMount(DeviceT* dev, const char* dstPath, const char* nodeName);

#endif
