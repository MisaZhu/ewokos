#ifndef PROTO_T_H
#define PROTO_T_H

#include <stdint.h>
#include <stdbool.h>
enum {
	PROTO_PKG = 0,
	PROTO_INT
};

#define PROTO_INT_NUM 4

typedef struct {
	uint32_t type;
	int32_t  ints[PROTO_INT_NUM];

	void*    data;
	uint32_t size;
	uint32_t total_size;
	uint32_t offset;
	uint32_t pre_alloc;
} proto_t;

#endif
