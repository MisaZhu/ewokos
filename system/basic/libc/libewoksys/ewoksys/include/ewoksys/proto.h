#ifndef PROTO_H
#define PROTO_H

#include <ewoksys/ewokdef.h>
#include <proto_t.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct proto_factor {
	struct proto_factor* (*init_data)(proto_t* proto, void* data, uint32_t size);
	struct proto_factor* (*reserve)(proto_t* proto, uint32_t size);
	struct proto_factor* (*init)(proto_t* proto);
	struct proto_factor* (*copy)(proto_t* proto, const void* data, uint32_t size);
	struct proto_factor* (*clear)(proto_t* proto);
	struct proto_factor* (*add)(proto_t* proto, const void* item, uint32_t size);
	struct proto_factor* (*addi)(proto_t* proto, int32_t i);
	struct proto_factor* (*adds)(proto_t* proto, const char* s);
	struct proto_factor* (*format)(proto_t* proto, const char* fmt, ... );
} proto_factor_t;

proto_t*        proto_new(void* data, uint32_t size);
void*           proto_read(proto_t* proto, int32_t *size);
int32_t         proto_read_to(proto_t* proto, void* to, int32_t size);
int32_t         proto_read_int(proto_t* proto);
const char*     proto_read_str(proto_t* proto);
int32_t         proto_read_proto(proto_t* proto, proto_t* to);
void            proto_reset(proto_t* proto);
void            proto_free(proto_t* proto);
proto_factor_t* get_proto_factor(void);
#define PF      get_proto_factor()

#ifdef __cplusplus
}
#endif

#endif
