#ifndef DEVICE_H
#define DEVICE_H

#include <types.h>

#define SDC_BLOCK_SIZE 1024

enum {
	DEV_UART = 0,
	DEV_KEVENT,
	DEV_KEYBOARD,
	DEV_MOUSE,
	DEV_FRAME_BUFFER,
	DEV_SDC
};

#define dev_typeid(type, id) ((type) << 16 | (id))

typedef struct {
	void* buffer;
	uint32_t buffer_size;
	uint32_t start;
	uint32_t size;
} dev_buffer_t;

int32_t dev_buffer_push(dev_buffer_t *buffer, char c, bool loop);
int32_t dev_buffer_pop(dev_buffer_t *buffer, char* c);

#endif
