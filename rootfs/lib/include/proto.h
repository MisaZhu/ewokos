#ifndef PROTO_H
#define PROTO_H

#include "types.h"
#include "kstring.h"

#define PROTO_BUFFER 128

typedef struct {
	void *data;
	uint32_t size;
	uint32_t totalSize;
	uint32_t offset;
	bool readOnly;
}proto_t;

void proto_init(proto_t* proto, void* data, uint32_t size);

proto_t* proto_new(void* data, uint32_t size);

void proto_add(proto_t* proto, void* item, uint32_t size);

void proto_add_int(proto_t* proto, int32_t v);

void proto_add_str(proto_t* proto, const char* v);

void* proto_read(proto_t* proto, uint32_t *size);

int32_t proto_read_int(proto_t* proto);

const char* proto_read_str(proto_t* proto);

void proto_clear(proto_t* proto);

void proto_free(proto_t* proto);

#endif
