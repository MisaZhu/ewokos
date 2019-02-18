#include "proto.h"
#include <kstring.h>
#include <mm/kmalloc.h>

void protoInit(ProtoT* proto, void* data, uint32_t size, uint32_t totalSize) {
	proto->data = data;
	proto->size = size;
	proto->totalSize = totalSize;
	proto->offset = 0;
}

void protoAdd(ProtoT* proto, void* item, uint32_t size) {
	uint32_t newSize = proto->size + size + 4;
	char* p = (char*)proto->data;
	if(proto->totalSize <= newSize) { 
		newSize +=  PROTO_BUFFER;
		proto->totalSize = newSize;
		p = (char*)kmalloc(newSize);
		if(proto->data != NULL) {
			memcpy(p, proto->data, proto->size);
			kmfree(proto->data);
		}
		proto->data = p;
	} 
	memcpy(p+proto->size, &size, 4);
	memcpy(p+proto->size+4, item, size);
	proto->size += (size + 4);
}

inline void protoAddInt(ProtoT* proto, int32_t v) {
	protoAdd(proto, (void*)&v, 4);
}

inline void protoAddStr(ProtoT* proto, const char* v) {
	protoAdd(proto, (void*)v, strlen(v)+1);
}

void* protoRead(ProtoT* proto, uint32_t *size) {
	*size = 0;
	if(proto->data == NULL || proto->size == 0 ||
			proto->offset >= proto->size)
		return NULL;
	char* p = ((char*)proto->data) + proto->offset;
	memcpy(size, p, 4);
	p += 4;
	proto->offset += (4 + *size);
	return p;
}

inline int32_t protoReadInt(ProtoT* proto) {
	uint32_t sz;
	void *p = protoRead(proto, &sz);
	if(p == NULL)
		return 0;
	return *(int*)p;
}

inline const char* protoReadStr(ProtoT* proto) {
	uint32_t sz;
	return (const char*)protoRead(proto, &sz);
}
void protoFree(ProtoT* proto) {
	if(proto->data != NULL)
		kmfree(proto->data);
	protoInit(proto, NULL, 0, 0);
}

