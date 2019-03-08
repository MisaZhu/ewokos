#include "proto.h"
#include <kstring.h>
#include <stdlib.h>

void protoInit(ProtoT* proto, void* data, uint32_t size) {
	proto->data = data;
	proto->size = size;
	proto->totalSize = size;
	proto->offset = 0;
	proto->readOnly = (data == NULL) ? false:true;
}

ProtoT* protoNew(void* data, uint32_t size) {
	ProtoT* ret = (ProtoT*)malloc(sizeof(ProtoT));
	protoInit(ret, data, size);
	return ret;
}

void protoAdd(ProtoT* proto, void* item, uint32_t size) {
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

	if(sz == 0)
		return NULL;
	return p+4;
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

void protoClear(ProtoT* proto) {
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

void protoFree(ProtoT* proto) {
	protoClear(proto);
	free(proto);
}
