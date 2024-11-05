#include <proto.h>
#include <stddef.h>
#include <kstring.h>
#include <mm/kmalloc.h>

#ifdef __cplusplus
extern "C" {
#endif

void proto_init(proto_t* proto) {
	proto->data = proto->buffer;
	proto->total_size = PROTO_BUFFER;
	proto->size = 0;
	proto->pre_alloc = 0;
	proto->offset = 0;
}

void proto_copy(proto_t* proto, const void* data, uint32_t size) {
	if(proto->total_size < size) {
		if(proto->data != NULL && proto->data != proto->buffer) 
			kfree(proto->data);
		proto->data = kmalloc(size);
		proto->total_size = size;
	}
	memcpy(proto->data, data, size);
	proto->size = size;
	proto->offset = 0;
	proto->pre_alloc = false;
}

void proto_clear(proto_t* proto) {
	proto->size = 0;
	proto->offset = 0;
	if(!proto->pre_alloc && proto->data != NULL && proto->data != proto->buffer)
		kfree(proto->data);
	proto->data = proto->buffer;
	proto->total_size = PROTO_BUFFER;
}

#ifdef __cplusplus
}
#endif

