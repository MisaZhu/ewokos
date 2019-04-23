#ifndef DEVICE_H
#define DEVICE_H

#include <types.h>
#include <devices.h>

typedef struct {
	char* buffer;
	uint32_t buffer_size;
	uint32_t offset;
	uint32_t size;
} dev_buffer_t;

int32_t dev_buffer_push(dev_buffer_t *buffer, char c, bool replace);
int32_t dev_buffer_pop(dev_buffer_t *buffer, char* c);
int32_t dev_read(int32_t type_id, void* buf, uint32_t size);
int32_t dev_write(int32_t type_id, void* buf, uint32_t size);

#endif
