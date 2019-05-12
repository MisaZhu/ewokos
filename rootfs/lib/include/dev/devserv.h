#ifndef DEVSERV_H
#define DEVSERV_H

#include <types.h>
#include <device.h>
#include <fsinfo.h>

typedef struct {
	int32_t (*mount)(uint32_t node, int32_t index);
	int32_t (*unmount)(uint32_t node);
	int32_t (*open)(uint32_t node, int32_t flags);
	int32_t (*close)(fs_info_t* info);
	int32_t (*remove)(fs_info_t* info);
	int32_t (*add)(uint32_t father_node, uint32_t node, const char* name, uint32_t type);
	int32_t (*write)(uint32_t node, void* buf, uint32_t size, int32_t seek);
	int32_t (*read)(uint32_t node, void* buf, uint32_t size, int32_t seek);
	int32_t (*dma)(uint32_t node, uint32_t *size);
	int32_t (*flush)(uint32_t node);
	void* (*ctrl)(uint32_t node, int32_t cmd, void* data, uint32_t size, int32_t* ret);
} device_t;

void dev_run(device_t* dev, int32_t argc, char** argv);

#endif
