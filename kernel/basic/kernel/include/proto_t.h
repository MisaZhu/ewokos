#ifndef PROTO_T_H
#define PROTO_T_H

#include <stdint.h>
#include <stdbool.h>
enum {
	PROTO_PKG = 0,
	PROTO_INT
};

typedef struct {
	uint32_t type;
	void *data;
	uint32_t size;
	uint32_t total_size;
	uint32_t offset; //also int value when type is PROTO_INT
	bool pre_alloc;
} proto_t;

#endif
