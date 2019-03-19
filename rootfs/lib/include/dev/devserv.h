#ifndef DEVSERV_H
#define DEVSERV_H

#include <types.h>

typedef struct {
	int (*mount)(uint32_t node, int32_t index);
	int (*unmount)(uint32_t node);
	int (*open)(uint32_t node, int32_t flags);
	int (*close)(uint32_t node);
	int (*add)(uint32_t node, const char* name);
	int (*write)(uint32_t node, void* buf, uint32_t size, int32_t seek);
	int (*read)(uint32_t node, void* buf, uint32_t size, uint32_t seek);
	int (*dma)(uint32_t node, uint32_t *size);
	int (*flush)(uint32_t node);
	void* (*ctrl)(uint32_t node, int32_t cmd, void* data, uint32_t size, int32_t* ret);
} DeviceT;

void devRun(DeviceT* dev, const char* devName, uint32_t index, const char* nodeName, bool file);

#endif
