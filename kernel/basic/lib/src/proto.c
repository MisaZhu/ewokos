#include <proto.h>
#include <stddef.h>
#include <kstring.h>
#include <mm/kmalloc.h>

#ifdef __cplusplus
extern "C" {
#endif

void proto_init(proto_t* proto) {
	memset(proto, 0, sizeof(proto_t));
}

void proto_copy(proto_t* proto, const void* data, uint32_t size) {
	if(proto->total_size < size) {
		if(proto->data != NULL)
			kfree(proto->data);
		proto->data = kmalloc(size);
		proto->total_size = size;
	}

	memcpy(proto->data, data, size);
	proto->size = size;
}

void proto_clear(proto_t* proto) {
	if(proto->data != NULL)
		kfree(proto->data);
	memset(proto, 0, sizeof(proto_t));
}

#ifdef __cplusplus
}
#endif

