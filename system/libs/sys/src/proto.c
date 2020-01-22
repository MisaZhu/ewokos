#include <proto.h>
#include <string.h>
#include <stdlib.h>

void proto_init(proto_t* proto, void* data, uint32_t size) {
	proto->id = -1;
	proto->data = data;
	proto->size = size;
	proto->total_size = size;
	proto->offset = 0;
	proto->read_only = (data == NULL) ? 0:1;
}

void proto_copy(proto_t* proto, const void* data, uint32_t size) {
	if(!proto->read_only && proto->data != NULL)
		free(proto->data);

	proto->data = malloc(size);
	memcpy(proto->data, data, size);
	proto->size = size;
	proto->total_size = size;
	proto->offset = 0;
	proto->read_only = 0;
}

proto_t* proto_new(void* data, uint32_t size) {
	proto_t* ret = (proto_t*)malloc(sizeof(proto_t));
	proto_init(ret, data, size);
	return ret;
}

void proto_add(proto_t* proto, const void* item, uint32_t size) {
	if(proto->read_only)
		return;

	uint32_t new_size = proto->size + size + 4;
	char* p = (char*)proto->data;
	if(proto->total_size <= new_size) { 
		new_size +=  PROTO_BUFFER;
		proto->total_size = new_size;
		p = (char*)malloc(new_size);
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

void* proto_read(proto_t* proto, int32_t *size) {
	if(size != NULL)
		*size = 0;
	if(proto->data == NULL || proto->size == 0 ||
			proto->offset >= proto->size)
		return NULL;
	char* p = ((char*)proto->data) + proto->offset;

	int32_t sz;
	memcpy(&sz, p, 4);
	proto->offset += (4 + sz);
	if(size != NULL)
		*size = sz;

	if(sz == 0)
		return NULL;
	return p+4;
}

int32_t proto_read_to(proto_t* proto, void* to, int32_t size) {
	int32_t sz;
	void *p = proto_read(proto, &sz);
	if(sz > size)
		sz = size;
	if(to == NULL || p == NULL || sz == 0)
		return 0;
		
	memcpy(to, p, sz);
	return sz;
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
	if(proto->read_only)
		return;

	proto->size = 0;
	proto->total_size = 0;
	proto->offset = 0;
	proto->read_only = 0;
	if(proto->data != NULL)
		free(proto->data);
	proto->data = NULL;
}

void proto_free(proto_t* proto) {
	proto_clear(proto);
	free(proto);
}
