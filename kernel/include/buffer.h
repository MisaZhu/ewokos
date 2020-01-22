#ifndef BUFFER_H
#define BUFFER_H

#include <types.h>

#define BUFFER_SIZE 1024

typedef struct {
	char buffer[BUFFER_SIZE];
	int32_t size;
	int32_t offset;
} buffer_t;

extern int32_t buffer_is_empty(buffer_t* buffer);
extern int32_t buffer_read(buffer_t* buffer, void* buf, int32_t size);
extern int32_t buffer_write(buffer_t* buffer, const void* buf, int32_t size);

#endif
