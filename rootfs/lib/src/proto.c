#include "proto.h"
#include <kstring.h>
#include <stdlib.h>

void proto_init(proto_t* proto, void* data, uint32_t size) {
	proto->data = data;
	proto->size = size;
	proto->totalSize = size;
	proto->offset = 0;
	proto->readOnly = (data == NULL) ? false:true;
}

proto_t* proto_new(void* data, uint32_t size) {
	proto_t* ret = (proto_t*)malloc(sizeof(proto_t));
	proto_init(ret, data, size);
	return ret;
}

void proto_add(proto_t* proto, void* item, uint32_t size) {
	if(proto->readOnly)
		return;

	uint32_t newSize = proto->size + size + 4;
	char* p = (char*)proto->data;
	if(proto->totalSize <= newSize) { 
		newSize +=  PROTO_BUFFER;
		proto->totalSize = newSize;
		p = (char*)malloc(newSize);
		if(proto->data != NULL) {
			memcpy(p, proto->data, proto->size);
			free(proto->data);
		}
		proto->data = p;
	} 
	memcpy(p+proto->size, &size, 4);
	if(size > 0 && item != NULL)
		memcpy(p+proto->size+4, item, size);
	proto->size += (size + 4);
}

inline void proto_add_int(proto_t* proto, int32_t v) {
	proto_add(proto, (void*)&v, 4);
}

inline void proto_add_str(proto_t* proto, const char* v) {
	proto_add(proto, (void*)v, strlen(v)+1);
}

void* proto_read(proto_t* proto, uint32_t *size) {
	if(size != NULL)
		*size = 0;
	if(proto->data == NULL || proto->size == 0 ||
			proto->offset >= proto->size)
		return NULL;
	char* p = ((char*)proto->data) + proto->offset;

	uint32_t sz;
	memcpy(&sz, p, 4);
	proto->offset += (4 + sz);
	if(size != NULL)
		*size = sz;

	if(sz == 0)
		return NULL;
	return p+4;
}

inline int32_t proto_read_int(proto_t* proto) {
	void *p = proto_read(proto, NULL);
	if(p == NULL)
		return 0;
	return *(int*)p;
}

inline const char* proto_read_str(proto_t* proto) {
	return (const char*)proto_read(proto, NULL);
}

void proto_clear(proto_t* proto) {
	if(proto->readOnly)
		return;

	if(proto->data != NULL)
		free(proto->data);

	proto->data = NULL;
	proto->size = 0;
	proto->totalSize = 0;
	proto->offset = 0;
	proto->readOnly = false;
}

void proto_free(proto_t* proto) {
	proto_clear(proto);
	free(proto);
}
