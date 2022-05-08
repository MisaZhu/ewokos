#ifndef PROTO_T_H
#define PROTO_T_H

#include <stdint.h>
#include <stdbool.h>
enum {
	PROTO_PKG = 0,
	PROTO_INT
};

#define PROTO_BUFFER  128

typedef struct {
	uint32_t type;

	char     buffer[PROTO_BUFFER];
	void*    data;

	uint32_t size;
	uint32_t total_size;
	uint32_t offset;
	uint32_t pre_alloc;
} proto_t;

#endif
