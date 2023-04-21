#ifndef PROTO_H
#define PROTO_H

#include <proto_t.h>

void proto_init(proto_t* proto);
void proto_copy(proto_t* proto, const void* data, uint32_t size);
void proto_clear(proto_t* proto);

#endif
