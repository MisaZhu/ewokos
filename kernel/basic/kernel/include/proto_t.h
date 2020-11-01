#ifndef PROTO_T_H
#define PROTO_T_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
	int32_t id;
	void *data;
	uint32_t size;
	uint32_t total_size;
	uint32_t offset;
	bool pre_alloc;
} proto_t;

#endif
