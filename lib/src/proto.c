#include "proto.h"
#include <kstring.h>

void protoInit(ProtoT* proto, MallocFuncT mlc, FreeFuncT fr, void* data, uint32_t size) {
	proto->data = data;
	proto->size = size;
	proto->totalSize = size;
	proto->offset = 0;
	proto->readOnly = (data == NULL) ? false:true;
	proto->malloc = mlc;
	proto->free = fr;
}

void protoAdd(ProtoT* proto, void* item, uint32_t size) {
	if(proto->readOnly)
		return;

	uint32_t newSize = proto->size + size + 4;
	char* p = (char*)proto->data;
	if(proto->totalSize <= newSize) { 
		newSize +=  PROTO_BUFFER;
		proto->totalSize = newSize;
		p = (char*)proto->malloc(newSize);
		if(proto->data != NULL) {
			memcpy(p, proto->data, proto->size);
			proto->free(proto->data);
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
	if(proto->data == NULL || proto->size == 0 ||
			proto->offset >= proto->size)
		return NULL;
	char* p = ((char*)proto->data) + proto->offset;

	uint32_t sz;
	memcpy(&sz, p, 4);
	proto->offset += (4 + sz);
	if(size != NULL)
		*size = sz;
	return p+4;;
}

inline int32_t protoReadInt(ProtoT* proto) {
	void *p = protoRead(proto, NULL);
	if(p == NULL)
		return 0;
	return *(int*)p;
}

inline const char* protoReadStr(ProtoT* proto) {
	return (const char*)protoRead(proto, NULL);
}

void protoFree(ProtoT* proto) {
	if(proto->readOnly)
		return;

	if(proto->data != NULL)
		proto->free(proto->data);

	proto->data = NULL;
	proto->size = 0;
	proto->totalSize = 0;
	proto->offset = 0;
	proto->readOnly = false;
}

