#ifndef DEV_H
#define DEV_H

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
  bool (*mount)(uint32_t index);
  bool (*unmount)(uint32_t index);
  bool (*open)(uint32_t index);
  int (*read)(uint32_t index, int, char*, uint32_t);
  int (*write)(uint32_t index, int, const char*, uint32_t);
  int (*info)(uint32_t index, FSInfoT* info);
  int (*setOpt)(uint32_t index, int cmd, int value);
  void (*close)(uint32_t index);

	char type[DEV_NAME_MAX];
} DeviceT;

void devInit(DeviceT* dev, const char* type);

const char* devGetServName(const char* devName);

bool devMount(DeviceT* dev, uint32_t index, const char* nodeName);

#endif
